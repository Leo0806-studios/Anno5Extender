# Anno5Extender bootstrap

This solution uses **MSBuild / Visual C++ v145**, the Windows SDK and Windows
PowerShell 5.1. Open `Anno5Extender.slnx` in Visual Studio and select **x64**.
The proxy rejects Win32 builds. No CMake or external package manager is needed.

From a Visual Studio developer shell:

```powershell
msbuild Anno5Extender.slnx /m /p:Configuration=Release /p:Platform=x64
```

Outputs are in `bin/x64/Release/` (or `Debug/`); intermediate files are in
`obj/x64/<configuration>/<project>/`. The proxy references the extender project
for build ordering, but does not link against its import library. Both use the
static CRT so a Debug bootstrap does not require a separately installed debug CRT.

## Runtime layout

The eventual game-directory layout is:

```text
Anno5.exe
upc_r2_loader64.dll                built proxy
upc_r2_loader64_anno_original.dll  preserved original Ubisoft DLL
Anno5Extender.dll                  built extender
Anno5Extender.proxy.log            best-effort runtime log
```

**Building does not install, rename, overwrite or launch anything in the game
directory.** The existing research proxy is a separate implementation. Preserve
the genuine original before replacing any installed proxy. Never rename an
existing proxy to become its own backend.

The supported original SHA-256 is
`658a140d0eb8023a24b173544b2a88e8441020e94f4362bbd74fa416a301bb72`.
This is a version-specific contract, not automatic support for future SDK DLLs.

## Export generation

`upc_r2_loader64/exports/upc_exports.json` is the reviewed input: 101 named
exports with their original ordinals, plus the backend basename and SHA-256.
The MSBuild `CustomBuild` step runs `tools/Generate-ProxyExports.ps1` before
compiling/linking and tracks the manifest, generator and project as inputs.
Changing these inputs or removing an output regenerates:

| Generated file | Purpose |
| --- | --- |
| `upc_r2_loader64.def` | Maps `UPC_Init @31` to the local wrapper |
| `proxy_exports.generated.h` | Backend identity and 100 `/export` linker directives |
| `upc_r2_loader64.exports.json` | Complete name/ordinal/target mapping for all 101 exports |

The `.def` and JSON mapping are copied beside the built DLL. The header stays in
the intermediate directory. The forwarders use linker directives because MSVC's
import-library pass rejected the equivalent aliases in a full `.def` in testing.
There are no hand-written function-signature guesses or generic trampolines for
those 100 exports: Windows forwards each directly to the backend export by name.
No research API export is added to the original interface.

## Init contract

The first `UPC_Init(uint32_t, uint32_t)` call checks/prepares the backend, then
loads the absolute sibling path `Anno5Extender.dll` and resolves:

```cpp
extern "C" __declspec(dllexport)
DWORD __cdecl Anno5Extender_Init() noexcept;
```

`ERROR_SUCCESS` means initialized. A nonzero Win32 status means initialization
failed or was partial. Do not throw across the export boundary. The thin bridge
in `Anno5Extender/INIT.cpp` catches C++ exceptions and calls `A5ext::Init()`;
**put your actual initialization in that currently empty function**.

The proxy attempts extender initialization once per process, before delegating
the initiating call to the genuine `UPC_Init`. It preserves the two arguments,
the native return value and the native call's incoming/outgoing `GetLastError`.
The game can restart into another process: each process independently loads its
own proxy/extender. Nothing here suppresses Ubisoft's relaunch or authorization.

`DllMain` only captures the proxy module handle. Initialization runs at the
`UPC_Init` call boundary, not from either DLL's `DllMain`. Call this boundary
from normal application code; it is not safe to invoke arbitrary initialization
from another module's `DllMain` either. Game objects may not exist this early.

Concurrent/reentrant callers do **not** wait for extender initialization to
finish; they delegate to the original SDK. This avoids an extender initializer
deadlocking when it joins a worker that itself calls `UPC_Init`. Do not interpret
this as a guarantee that the native SDK supports arbitrary concurrent or recursive
initialization. Extender initialization must tolerate these callers proceeding,
and must not assume the native SDK has already initialized.

Missing extender DLLs/exports and nonzero init results are logged while the
native SDK call continues. A loaded extender is retained even after failed init,
because partial initialization may have installed live callbacks. There is no
automatic retry or hot reload. Backend hash/load/export failures fail fast rather
than inventing a UPC result. Validation failures are cached for the process.

The hash is checked at the wrapped call, **not before Windows resolves forwarded
imports**: the original may already have loaded. It is a compatibility check,
not a security boundary. Installers should validate the original before launch.

`A5ext::Shutdown()` and `Bootstrap::ShutdownFn` are suggested future lifecycle
declarations only. They are not implemented, exported or invoked. Before adding
unloading, define a shutdown result that confirms hooks are removed, callbacks
and workers are quiescent, and no game code can still enter the module. Never
attempt that shutdown from `DllMain`. A versioned API-query export can be added
when your public extender/address-library contract is defined.

The best-effort log also goes to `OutputDebugStringA`; failure to create a log
file in a read-only installation does not block native delegation.

## Verification

The optional Python test driver uses MSBuild for all native fixture builds:

```powershell
python upc_r2_loader64/tests/verify.py --binary bin/x64/Debug/upc_r2_loader64.dll --msbuild F:/VSS/MSBuild/Current/Bin/MSBuild.exe
```

It checks the production PE against the manifest, then builds an isolated fake
backend and extender under the repository's ignored `analysis/` directory.
Only this fixture build uses a manifest containing the mock backend's hash;
the production code has no runtime validation bypass. Subprocesses test native
forwarding, argument/return/error preservation, once-only concurrent/reentrant
initialization, failed init, a missing extender and rejection of a modified mock
backend. It also checks incremental generation, recreation of missing generated
outputs and rejection of duplicate ordinals. No game or real UPC SDK is
loaded. Python is only needed for these optional tests, not normal builds.
