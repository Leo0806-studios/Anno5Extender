param(
    [Parameter(Mandatory=$true)][string]$Manifest,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$data=Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
if ($data.schema_version -ne 1 -or $data.machine -ne 'AMD64' -or
    $data.original_sha256 -notmatch '^[a-f0-9]{64}$' -or
    $data.backend_module -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') { throw 'Invalid proxy manifest identity' }
$exports=@($data.exports | Sort-Object ordinal)
if ($exports.Count -ne 101) { throw 'This reviewed UPC interface requires exactly 101 exports' }
$names=@{};$ordinals=@{}
foreach ($entry in $exports) {
    if ($entry.name -notmatch '^UPC_[A-Za-z0-9_]+$' -or $entry.ordinal -lt 1 -or $entry.ordinal -gt 65535 -or
        [double]$entry.ordinal -ne [int]$entry.ordinal) { throw 'Invalid export' }
    if ($names.ContainsKey($entry.name) -or $ordinals.ContainsKey([int]$entry.ordinal)) { throw 'Duplicate export name/ordinal' }
    $names[$entry.name]=$true;$ordinals[[int]$entry.ordinal]=$true
}
$init=@($exports | Where-Object name -eq 'UPC_Init')
if ($init.Count -ne 1 -or $init[0].ordinal -ne 31) { throw 'Expected reviewed UPC_Init at ordinal 31' }
$null=New-Item -ItemType Directory -Path $OutputDirectory -Force
$def=[Collections.Generic.List[string]]::new()
$def.Add('; Generated from exports/upc_exports.json. Do not edit.')
$def.Add('LIBRARY upc_r2_loader64');$def.Add('EXPORTS')
$mapping=@()
# MSVC's import-library pass rejects these forwarder aliases in a full .def.
# Emit forwarders as linker directives; reserve the .def for the local wrapper.
$pragmas=[Collections.Generic.List[string]]::new()
foreach ($entry in $exports) {
    $wrapped=$entry.name -eq 'UPC_Init'
    $target=if ($wrapped) {'Proxy_UPC_Init'} else {$data.backend_module+'.'+$entry.name}
    if ($wrapped) { $def.Add(('  {0}={1} @{2}' -f $entry.name,$target,$entry.ordinal)) }
    else { $pragmas.Add(('#pragma comment(linker, "/export:{0}={1},@{2}")' -f $entry.name,$target,$entry.ordinal)) }
    $mapping += [ordered]@{name=$entry.name;ordinal=[int]$entry.ordinal;target=$target;forwarder=(-not $wrapped)}
}
$header=@"
// Generated. The hash identifies the reviewed original, not this proxy.
#pragma once
namespace A5ext::Proxy::Generated {
inline constexpr wchar_t BackendFileName[]=L"$($data.backend_module).dll";
inline constexpr char BackendSha256[]="$($data.original_sha256)";
inline constexpr unsigned ExportCount=101;
inline constexpr unsigned InitOrdinal=31;
}
"@
$encoding=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'upc_r2_loader64.def'),(($def -join "`n")+"`n"),$encoding)
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'proxy_exports.generated.h'),$header+"`n",$encoding)
[IO.File]::AppendAllText((Join-Path $OutputDirectory 'proxy_exports.generated.h'),(($pragmas -join "`n")+"`n"),$encoding)
$receipt=[ordered]@{schema_version=1;machine=$data.machine;original_sha256=$data.original_sha256;backend_module=$data.backend_module;exports=$mapping}
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'upc_r2_loader64.exports.json'),(($receipt|ConvertTo-Json -Depth 6)+"`n"),$encoding)
Write-Output 'Generated UPC proxy: 101 exports, UPC_Init wrapper at ordinal 31, 100 forwarders.'
