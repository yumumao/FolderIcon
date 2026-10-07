[CmdletBinding()]
param(
    [string]$Zig = 'zig',
    [string]$BuildRoot = (Join-Path ([IO.Path]::GetTempPath()) 'foldericon-build'),
    [string]$OutputDirectory = '',
    [switch]$TestsOnly
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!$OutputDirectory) { $OutputDirectory = Join-Path $root 'dist' }
$zigCommand = (Get-Command $Zig -ErrorAction Stop).Source
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
foreach ($dir in @($BuildRoot, "$BuildRoot/tmp", "$BuildRoot/cache/global", "$BuildRoot/cache/local")) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}
$names = @('TEMP', 'TMP', 'ZIG_GLOBAL_CACHE_DIR', 'ZIG_LOCAL_CACHE_DIR')
$oldEnvironment = @{}
foreach ($name in $names) { $oldEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$env:TEMP = "$BuildRoot/tmp"; $env:TMP = $env:TEMP
$env:ZIG_GLOBAL_CACHE_DIR = "$BuildRoot/cache/global"
$env:ZIG_LOCAL_CACHE_DIR = "$BuildRoot/cache/local"
Push-Location $BuildRoot
try {
    & $zigCommand c++ "$root/tests/core_test.cpp" -std=c++17 -O2 -UNDEBUG -o "$BuildRoot/core-test.exe"
    if ($LASTEXITCODE) { throw 'Core test compilation failed' }
    & "$BuildRoot/core-test.exe"
    if ($LASTEXITCODE) { throw 'Core tests failed' }
    if ($TestsOnly) { return }
    & $zigCommand c++ "$root/src/main.cpp" "$root/src/shell.cpp" "$root/src/panel.cpp" "$root/src/settings.cpp" -std=c++17 -Os -s -municode '-Wl,--subsystem,windows' -o "$BuildRoot/FolderIconCN.exe" -luser32 -lshell32 -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -loleacc -luuid -ladvapi32 -ldwmapi -lshlwapi -lgdiplus
    if ($LASTEXITCODE) { throw 'Application compilation failed' }
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    Copy-Item -LiteralPath "$BuildRoot/FolderIconCN.exe" -Destination "$OutputDirectory/FolderIconCN.exe"
    # Never overwrite the user's local INI or icons.
    Get-Item -LiteralPath "$OutputDirectory/FolderIconCN.exe" | Select-Object FullName, Length
    Get-FileHash -LiteralPath "$OutputDirectory/FolderIconCN.exe" -Algorithm SHA256
} finally {
    Pop-Location
    foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name, $oldEnvironment[$name], 'Process') }
}
