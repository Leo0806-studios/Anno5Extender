"""Build isolated MSBuild fixtures; never load the real UPC SDK or launch Anno."""
import argparse
import concurrent.futures
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / 'Tools'))
from extract_cpp_rtti import CppImage
from inspect_chainload_candidates import exports, imports


def verify_exports(binary, manifest):
    pe = CppImage(binary)
    actual = exports(pe)
    expected = {(e['name'], e['ordinal']) for e in manifest['exports']}
    assert {(e['names'][0], e['ordinal']) for e in actual if len(e['names']) == 1} == expected
    assert len(actual) == 101
    for e in actual:
        if e['names'] == ['UPC_Init']:
            assert e['kind'] == 'executable-section' and e['ordinal'] == 31
        else:
            assert e['forwarder'] == manifest['backend_module'] + '.' + e['names'][0]
    assert not any(e['module'].lower() == 'anno5extender.dll' for e in imports(pe))


def child(folder, mode):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.SetErrorMode(3)  # No critical-error or crash dialog in fixture subprocesses.
    with os.add_dll_directory(str(folder)):
        proxy = ctypes.CDLL(str(folder / 'upc_r2_loader64.dll'), winmode=0x1100, use_last_error=True)
        forward = proxy.UPC_AchievementImageFree
        forward.argtypes = [ctypes.c_uint64] * 4
        forward.restype = ctypes.c_uint64
        values = (0x1111222233334444, 0xaaaabbbbccccdddd, 0x123456789abcdef0, 0xfedcba9876543210)
        assert forward(*values) == values[0] ^ values[1] ^ values[2] ^ values[3]
        init = proxy.UPC_Init
        init.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
        init.restype = ctypes.c_int
        if mode == 'bad-hash':
            init(1, 2)
            raise AssertionError('Invalid backend hash was accepted')
        def invoke(i):
            ctypes.set_last_error(0x4321)
            result = init(i, 0x80000000)
            assert result == ctypes.c_int(i ^ 0x80000000 ^ 0xdeadbeef).value
            assert ctypes.get_last_error() == 0x8765
        if mode == 'concurrent':
            with concurrent.futures.ThreadPoolExecutor(max_workers=16) as pool:
                list(pool.map(invoke, range(32)))
            count = 32
        else:
            invoke(1); invoke(2)
            count = 2 + (mode == 'reentrant')
        backend = ctypes.CDLL(str(folder / 'upc_r2_loader64_anno_original.dll'), winmode=0x1100)
        assert backend.FixtureCalls() == count
        assert backend.FixtureBadError() == 0
        if mode != 'missing':
            extender = ctypes.CDLL(str(folder / 'Anno5Extender.dll'), winmode=0x1100)
            assert extender.FixtureCalls() == 1
        log = (folder / 'Anno5Extender.proxy.log').read_text()
        event = 'extender_load_failed' if mode == 'missing' else 'extender_init_finished'
        assert event in log
        if mode == 'failed': assert 'extender_init_finished status=5' in log
        if mode == 'reentrant': assert 'extender_init_finished status=0' in log
    print('PASS', mode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--msbuild', type=Path)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--child', type=Path)
    parser.add_argument('--mode')
    args = parser.parse_args()
    if args.child:
        child(args.child, args.mode)
        return
    manifest = json.loads((PROJECT / 'exports/upc_exports.json').read_text())
    if args.binary:
        verify_exports(args.binary, manifest)
        print('PASS production PE: 101 exports; 100 exact forwarders; no static extender import')
    if not args.msbuild: return
    folder = Path(tempfile.mkdtemp(prefix='upc-msbuild-', dir=ROOT / 'analysis'))
    print('Isolated fixture directory:', folder, flush=True)
    def build(project, **properties):
        command = [str(args.msbuild), str(project), '/nologo', '/verbosity:quiet', '/p:Configuration=Debug', '/p:Platform=x64']
        command += ['/p:' + k + '=' + str(v) for k, v in properties.items()]
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        if result.returncode: raise RuntimeError(result.stdout + result.stderr)
    definition = folder / 'backend.def'
    definition.write_text('EXPORTS\n' + '\n'.join(
        f"{e['name']}={'FixtureUpcInit' if e['name'] == 'UPC_Init' else 'FixtureForward'} @{e['ordinal']}"
        for e in manifest['exports']))
    build(HERE / 'Fixture.vcxproj', FixtureKind='backend', FixtureDef=definition,
          OutDir=str(folder) + os.sep, IntDir=str(folder / 'backend-obj') + os.sep,
          TargetName='upc_r2_loader64_anno_original')
    build(HERE / 'Fixture.vcxproj', FixtureKind='extender',
          OutDir=str(folder) + os.sep, IntDir=str(folder / 'extender-obj') + os.sep, TargetName='Anno5Extender')
    manifest['original_sha256'] = hashlib.sha256((folder / 'upc_r2_loader64_anno_original.dll').read_bytes()).hexdigest()
    test_manifest = folder / 'fixture-exports.json'
    test_manifest.write_text(json.dumps(manifest))
    def build_proxy():
        build(PROJECT / 'upc_r2_loader64.vcxproj', UpcExportManifest=test_manifest,
              BuildProjectReferences='false', OutDir=str(folder) + os.sep, IntDir=str(folder / 'proxy-obj') + os.sep)
    build_proxy()
    generated = folder / 'proxy-obj/generated/proxy_exports.generated.h'
    timestamp = generated.stat().st_mtime_ns
    build_proxy()
    assert generated.stat().st_mtime_ns == timestamp, 'Unchanged build reran export generator'
    generated.unlink()
    build_proxy()
    assert generated.is_file(), 'Missing generated output was not recreated'
    print('PASS incremental generation and deleted-output recovery', flush=True)
    invalid = dict(manifest, exports=[dict(e) for e in manifest['exports']])
    invalid['exports'][1]['ordinal'] = invalid['exports'][0]['ordinal']
    invalid_path = folder / 'invalid.json'
    invalid_path.write_text(json.dumps(invalid))
    rejected_output = folder / 'must-not-be-written'
    result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
                             '-File', str(PROJECT / 'tools/Generate-ProxyExports.ps1'),
                             '-Manifest', str(invalid_path), '-OutputDirectory', str(rejected_output)],
                            capture_output=True, text=True, timeout=20)
    assert result.returncode and not rejected_output.exists(), 'Duplicate ordinal was accepted'
    print('PASS malformed manifest rejected before writing outputs', flush=True)
    verify_exports(folder / 'upc_r2_loader64.dll', manifest)
    for mode in ('normal', 'concurrent', 'reentrant', 'failed', 'missing', 'bad-hash'):
        case = folder / mode
        case.mkdir()
        for name in ('upc_r2_loader64.dll', 'upc_r2_loader64_anno_original.dll', 'Anno5Extender.dll'):
            if mode != 'missing' or name != 'Anno5Extender.dll': shutil.copy2(folder / name, case / name)
        if mode == 'bad-hash':
            with (case / 'upc_r2_loader64_anno_original.dll').open('ab') as stream:
                stream.write(b'fixture hash mismatch')
        env = dict(os.environ, ANNO_FIXTURE_MODE=mode)
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--child', str(case), '--mode', mode],
                                env=env, capture_output=True, text=True, timeout=20, creationflags=subprocess.CREATE_NO_WINDOW)
        if mode == 'bad-hash':
            assert result.returncode != 0
            assert 'fatal_backend_error status=1306' in (case / 'Anno5Extender.proxy.log').read_text()
            print('PASS backend hash mismatch fails fast', flush=True)
            continue
        if result.returncode: raise RuntimeError(mode + '\n' + result.stdout + result.stderr)
        print(result.stdout.strip(), flush=True)


if __name__ == '__main__':
    main()
