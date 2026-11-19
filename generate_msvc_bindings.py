"""Extract MSVC decorated declarations with Clang; optionally emit import slots.

Example (run in a VS x64 Developer Command Prompt):
  python generate_msvc_bindings.py Anno5Extender/CBehaviourAutoBuild.h \
    --ast-filter A5Ext --match CBehaviourAutoBuild --json names.json \
    --bindings bindings.cpp -- -IAnno5Extender

Only dllimport functions are selected by default. --all-functions includes other
linkable function declarations. using/typedef are resolved by Clang, not regex.
Class-format slots start Unbound: bind every used slot BEFORE enabling callers.
No native RVAs, executable identity checks or game object layouts are inferred.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import shutil
import subprocess
import sys

FUNCTIONS = {'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl',
             'CXXDestructorDecl', 'CXXConversionDecl'}
CONTEXTS = {'NamespaceDecl', 'CXXRecordDecl', 'RecordDecl',
            'ClassTemplateSpecializationDecl', 'ClassTemplatePartialSpecializationDecl'}


def documents(text):
    """Clang's AST name filter can produce several concatenated JSON roots."""
    decoder = json.JSONDecoder()
    position = 0
    while position < len(text):
        while position < len(text) and text[position].isspace():
            position += 1
        if position == len(text):
            break
        root, position = decoder.raw_decode(text, position)
        yield root


def extract(roots, matches, all_functions):
    contexts, candidates = {}, []

    def visit(node, context=(), imported_class=False, owner=None):
        if node.get('isImplicit'):
            return
        kind, name = node.get('kind'), node.get('name', '')
        children = node.get('inner', [])
        own_import = any(c.get('kind') == 'DLLImportAttr' for c in children)
        if kind in CONTEXTS:
            context = context + ((name,) if name else ())
            if node.get('id'):
                contexts[node['id']] = context
            imported_class = own_import if kind != 'NamespaceDecl' else False
            if kind != 'NamespaceDecl':
                owner = '::'.join(context)
        if kind in FUNCTIONS and node.get('mangledName'):
            candidates.append((node, context, own_import or imported_class, owner))
        for child in children:
            visit(child, context, imported_class, owner)

    for root in roots:
        visit(root)
    result = {}
    for node, context, imported, owner in candidates:
        context = contexts.get(node.get('parentDeclContextId'), context)
        qualified = '::'.join(context + (node['name'],))
        if not all_functions and not imported:
            continue
        if matches and not any(match in qualified for match in matches):
            continue
        symbol = node['mangledName']
        # This generator targets function import POINTER symbols on MSVC x64.
        slot = 'AnnoGeneratedImport_' + hashlib.sha256(symbol.encode()).hexdigest()[:20]
        result[symbol] = dict(qualified_name=qualified, kind=node['kind'],
                              owner_class=owner, method_name=node['name'],
                              signature=node.get('type', {}).get('qualType', ''),
                              dllimport=imported, mangled_name=symbol,
                              import_symbol='__imp_' + symbol, slot=slot)
    return sorted(result.values(), key=lambda r: (r['qualified_name'], r['mangled_name']))


def class_bindings(rows, header, rvas, template=False):
    groups = {}
    for row in rows:
        owner = row['owner_class']
        if not owner:
            raise ValueError('Class format requires class members; use --format raw for free functions')
        groups.setdefault(owner, []).append(row)
    short_counts = {}
    for owner in groups:
        short = owner.split('::')[-1]
        if not re.fullmatch(r'[A-Za-z_]\w*', short, flags=re.ASCII):
            raise ValueError('Template/special class names require a verified concrete owner declaration')
        short_counts[short] = short_counts.get(short, 0) + 1
    outputs = {}
    for owner, methods in sorted(groups.items()):
        short = owner.split('::')[-1]
        stem = short if short_counts[short] == 1 else short + '_' + hashlib.sha256(owner.encode()).hexdigest()[:8]
        names = {}
        for row in methods:
            method = row['method_name']
            method = re.sub(r'\W', '_', method[:1].lower() + method[1:])
            row['slot'] = stem + '_' + method
            names[row['slot']] = names.get(row['slot'], 0) + 1
        for row in methods:
            if names[row['slot']] > 1:
                row['slot'] += '_' + hashlib.sha256(row['mangled_name'].encode()).hexdigest()[:8]
        lines = ['#include <intrin.h>', '#include ' + json.dumps(header), '#include <cstdint>', '',
                 '// Generated bindings: validate executable/signatures before initializing.',
                 '// Compile once; do not provide competing import-library symbols.']
        for row in methods:
            option = '/alternatename:' + row['import_symbol'] + '=' + row['slot']
            lines.append('#pragma comment(linker, ' + json.dumps(option) + ')')
        lines += ['', 'extern "C" {']
        for row in methods:
            lines.append('    A5Ext::AddressLibrary::DllEntry ' + row['slot'] + ' = &A5Ext::AddressLibrary::Unbound;')
        lines += ['}', '', 'namespace A5Ext::AddressLibrary {', '', 'template <>',
                  'class BindingStatic<' + owner + '> {', 'public:']
        if template:
            lines += ['    // Deliberate compile blocker: never run a zero-RVA binding.',
                      '    static_assert(false, "Unfinished binding template: fill every zero RVA and remove this assertion, or regenerate with --rva-map.");']
        lines.append('    BindingStatic() {')
        for row in methods:
            symbol = row['mangled_name']
            if template:
                lines.append('        ' + row['slot'] + ' = reinterpret_cast<DllEntry>(this->GetAddress(0x0)); // TODO: verified RVA')
            elif symbol in rvas:
                lines.append('        ' + row['slot'] + ' = reinterpret_cast<DllEntry>(this->GetAddress(' + hex(rvas[symbol]) + '));')
            else:
                lines += ['        // TODO: verified RVA for ' + row['qualified_name'] + ' ' + row['signature'],
                          '        // ' + row['slot'] + ' = reinterpret_cast<DllEntry>(this->GetAddress(/* RVA */));']
        lines += ['    }', '', 'private:',
                  '    // A specialization does not inherit the primary template helper.',
                  '    std::uintptr_t GetAddress(std::uintptr_t rva) const {',
                  '        return Module::GetInstance().GetBaseAddress() + rva;',
                  '    }', '};', '',
                  '// No global instance: call explicitly AFTER executable validation.',
                  '// Declare this function in the resolver TU before calling it.',
                  'void Initialize_' + stem + 'Bindings() {',
                  '    static BindingStatic<' + owner + '> binding;', '    (void)binding;',
                  '}', '', '} // namespace A5Ext::AddressLibrary', '']
        outputs[stem + '.cpp'] = '\n'.join(lines)
    return outputs


def bindings_source(rows):
    lines = [
        '// Generated MSVC x64 manual import bindings. Compile this .cpp ONCE.',
        '// Link the .obj directly, or use /WHOLEARCHIVE if storing it in a .lib.',
        '// Do not also provide competing import-library definitions.',
        '// NULL slots MUST be initialized before any imported call/static initializer.',
        '// Imported declarations stay in your own headers; this file defines slots only.',
        '#include <cstddef>', '#include <cstdint>',
        'static_assert(sizeof(void*) == 8, "MSVC x64 bindings required");',
        'extern "C" {',
    ]
    for row in rows:
        lines.append('    std::uintptr_t ' + row['slot'] + ' = 0;')
    lines.append('}')
    for row in rows:
        option = '/alternatename:' + row['import_symbol'] + '=' + row['slot']
        lines.append('#pragma comment(linker, ' + json.dumps(option) + ')')
    lines += [
        'namespace AnnoGeneratedBindings {',
        'struct Binding { const char* symbol; std::uintptr_t* target; };',
        'Binding entries[] = {',
    ]
    for row in rows:
        lines.append('    {' + json.dumps(row['mangled_name']) + ', &' + row['slot'] + '},')
    lines += ['};', 'const std::size_t count = sizeof(entries) / sizeof(entries[0]);',
              '// After validating executable + signature: *entries[i].target = imageBase + verifiedRva;',
              '// Publish readiness only after ALL bindings are complete; do not rebind live slots.',
              '} // namespace AnnoGeneratedBindings', '']
    return '\n'.join(lines)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    extra = []
    if '--' in argv:
        split = argv.index('--')
        argv, extra = argv[:split], argv[split + 1:]
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('source', type=Path)
    parser.add_argument('--clang', default='clang++', help='clang++ executable (not clang-cl)')
    parser.add_argument('--std', default='c++20')
    parser.add_argument('--match', action='append', default=[], help='Qualified-name substring; repeat for OR matching')
    parser.add_argument('--ast-filter', help='Clang AST name filter; use outer namespace to retain qualification')
    parser.add_argument('--all-functions', action='store_true')
    parser.add_argument('--json', type=Path, help='Write inventory here instead of stdout')
    parser.add_argument('--bindings', type=Path, help='Write generated slot definitions/aliases .cpp')
    parser.add_argument('--format', choices=['class', 'raw'], default='class')
    parser.add_argument('--output-dir', type=Path, help='Write one ClassName.cpp per class')
    parser.add_argument('--header', help='Header include in class output; defaults to input header basename')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--rva-map', type=Path, help='JSON object mapping exact decorated names to positive RVAs (integer or hex string)')
    mode.add_argument('--template', action='store_true', help='Zero-RVA class template with compile blocker (default without --rva-map)')
    parser.add_argument('--force', action='store_true', help='Allow replacing existing output files')
    args = parser.parse_args(argv)
    source = args.source.resolve(strict=True)
    executable = shutil.which(args.clang)
    if not executable:
        parser.error('clang++ not found; install LLVM or pass --clang PATH')
    for destination in (args.json, args.bindings):
        if destination and destination.resolve() == source:
            parser.error('An output path must not overwrite the input source')
    if args.json and args.bindings and args.json.resolve() == args.bindings.resolve():
        parser.error('JSON and C++ outputs must have different paths')
    if args.bindings and args.output_dir:
        parser.error('Choose --bindings or --output-dir')
    if args.output_dir and args.format != 'class':
        parser.error('--output-dir requires --format class')
    if args.template and args.format != 'class':
        parser.error('--template requires --format class')
    command = [executable, '--target=x86_64-pc-windows-msvc', '-x', 'c++',
               '-std=' + args.std, '-fsyntax-only', '-Xclang', '-ast-dump=json']
    if args.ast_filter:
        command += ['-Xclang', '-ast-dump-filter', '-Xclang', args.ast_filter]
    # Pass preprocessing/include options here, not output/action/target overrides.
    if any(not flag.startswith(('-I', '-D', '-U', '-isystem', '-include', '-f', '-W', '-m', '-stdlib=', '-std=')) for flag in extra):
        parser.error('After -- use joined compiler options, e.g. -IC:/SDK/include -DFEATURE=1')
    command += extra + [str(source)]
    completed = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace')
    if completed.stderr:
        print(completed.stderr, end='', file=sys.stderr)
    if completed.returncode:
        return completed.returncode  # Never publish a misleading partial AST inventory.
    rows = extract(list(documents(completed.stdout)), args.match, args.all_functions)
    if not rows:
        print('No matching decorated declarations; no output files were written.', file=sys.stderr)
        return 2
    output_files = {}
    if args.bindings or args.output_dir:
        if args.format == 'class':
            rvas = {}
            if args.rva_map:
                mapping = json.loads(args.rva_map.read_text(encoding='utf-8-sig'))
                if not isinstance(mapping, dict):
                    parser.error('--rva-map must contain a JSON object')
                for symbol, value in mapping.items():
                    if isinstance(value, bool) or not isinstance(value, (str, int)):
                        parser.error('Each RVA must be a positive integer or hex string')
                    try:
                        rva = int(value, 0) if isinstance(value, str) else value
                    except ValueError:
                        parser.error('Invalid RVA for ' + symbol)
                    if not 0 < rva <= 0xffffffff:
                        parser.error('RVA out of PE32+ range for ' + symbol)
                    if symbol not in {row['mangled_name'] for row in rows}:
                        parser.error('RVA map symbol not selected: ' + symbol)
                    rvas[symbol] = rva
            header = args.header or source.with_suffix('.h').name
            if source.suffix.lower() in ('.h', '.hpp', '.hxx') and not args.header:
                header = source.name
            try:
                generated = class_bindings(rows, header, rvas, template=args.rva_map is None)
            except ValueError as error:
                parser.error(str(error))
            if args.bindings:
                if len(generated) != 1:
                    parser.error('--bindings requires one selected class; use --match or --output-dir')
                output_files[args.bindings] = next(iter(generated.values()))
            else:
                output_files = {args.output_dir / name: text for name, text in generated.items()}
        else:
            output_files[args.bindings] = bindings_source(rows)
    if any(path.resolve() == source or (args.json and path.resolve() == args.json.resolve()) or
           (args.rva_map and path.resolve() == args.rva_map.resolve()) for path in output_files):
        parser.error('Generated output would overwrite an input or the JSON inventory')
    all_outputs = list(output_files) + ([args.json] if args.json else [])
    inputs = {source} | ({args.rva_map.resolve()} if args.rva_map else set())
    if any(path.resolve() in inputs for path in all_outputs):
        parser.error('An output would overwrite an input')
    if not args.force and any(path.exists() for path in all_outputs):
        parser.error('Output already exists; choose a new destination or use --force')
    report = dict(schema_version=1, source=str(source), source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  generation_mode=('template' if args.rva_map is None else 'rva-map') if args.format == 'class' else 'raw',
                  target='x86_64-pc-windows-msvc', command=command, symbols=rows,
                  limitations=['Clang Microsoft ABI names; verify against your MSVC toolset.',
                               'Only declarations with AST mangledName; dependent templates and implicit symbols are not enumerated.',
                               'Constructor/destructor thunk variants may need additional compiler-emitted references.',
                               'No game RVAs or native signatures are discovered.'])
    rendered = json.dumps(report, indent=2, ensure_ascii=False) + '\n'
    if args.json:
        args.json.write_text(rendered, encoding='utf-8')
    else:
        print(rendered, end='')
    if args.output_dir:
        args.output_dir.mkdir(parents=True, exist_ok=True)
    for path, content in output_files.items():
        path.write_text(content, encoding='utf-8')
    print('Extracted {} symbols.'.format(len(rows)), file=sys.stderr)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
