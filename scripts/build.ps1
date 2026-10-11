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
    & $zigCommand c++ "$root/tests/context_menu_test.cpp" "$root/src/context_menu.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/context-menu-test.exe" -luser32 -lshell32 -ladvapi32
    if ($LASTEXITCODE) { throw 'Context menu test compilation failed' }
    & "$BuildRoot/context-menu-test.exe"
    if ($LASTEXITCODE) { throw 'Context menu tests failed' }
    & $zigCommand c++ "$root/tests/icon_cache_test.cpp" "$root/src/icon_cache.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/icon-cache-test.exe" -lgdi32 -luser32 -lbcrypt
    if ($LASTEXITCODE) { throw 'Icon cache test compilation failed' }
    & "$BuildRoot/icon-cache-test.exe" (Join-Path $BuildRoot ("cache-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Icon cache tests failed' }
    & $zigCommand c++ "$root/tests/folder_sort_test.cpp" "$root/src/folder_sort.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/folder-sort-test.exe" -lshlwapi -lbcrypt
    if ($LASTEXITCODE) { throw 'Folder sort test compilation failed' }
    & "$BuildRoot/folder-sort-test.exe" (Join-Path $BuildRoot ("sort-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Folder sort tests failed' }
    & $zigCommand c++ "$root/tests/folder_rules_test.cpp" "$root/src/folder_rules.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/folder-rules-test.exe" -lshlwapi
    if ($LASTEXITCODE) { throw 'Folder rules test compilation failed' }
    & "$BuildRoot/folder-rules-test.exe" (Join-Path $BuildRoot ("rules-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Folder rules tests failed' }
    & $zigCommand c++ "$root/tests/folder_navigation_test.cpp" -std=c++17 -O2 -UNDEBUG -o "$BuildRoot/navigation-test.exe"
    if ($LASTEXITCODE) { throw 'Navigation test compilation failed' }
    & "$BuildRoot/navigation-test.exe"
    if ($LASTEXITCODE) { throw 'Navigation tests failed' }
    & $zigCommand c++ "$root/tests/config_store_test.cpp" "$root/src/config_store.cpp" "$root/src/shell.cpp" "$root/src/folder_rules.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/config-store-test.exe" -luser32 -lshell32 -lgdi32 -lcomctl32 -lole32 -loleaut32 -loleacc -luuid -ladvapi32 -lshlwapi
    if ($LASTEXITCODE) { throw 'Config store test compilation failed' }
    & "$BuildRoot/config-store-test.exe" (Join-Path $BuildRoot ("config-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Config store tests failed' }
    & $zigCommand c++ "$root/tests/folder_badge_test.cpp" "$root/src/folder_badge.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/folder-badge-test.exe" -luser32 -lgdi32 -lgdiplus -lshell32 -lole32 -luuid -lshlwapi
    if ($LASTEXITCODE) { throw 'Folder badge test compilation failed' }
    & "$BuildRoot/folder-badge-test.exe" (Join-Path $BuildRoot ("badge-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Folder badge tests failed' }
    & $zigCommand c++ "$root/tests/entry_actions_test.cpp" "$root/src/entry_actions.cpp" "$root/src/config_store.cpp" "$root/src/shell.cpp" "$root/src/folder_rules.cpp" "$root/src/folder_badge.cpp" "$root/src/context_menu.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/entry-actions-test.exe" -luser32 -lshell32 -lgdi32 -lgdiplus -lcomctl32 -lole32 -loleaut32 -loleacc -luuid -ladvapi32 -lshlwapi
    if ($LASTEXITCODE) { throw 'Entry actions test compilation failed' }
    & "$BuildRoot/entry-actions-test.exe" (Join-Path $BuildRoot ("entry-test-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Entry actions tests failed' }
    & $zigCommand c++ "$root/tests/panel_interaction_test.cpp" -std=c++17 -O2 -UNDEBUG -o "$BuildRoot/panel-interaction-test.exe"
    if ($LASTEXITCODE) { throw 'Panel interaction test compilation failed' }
    & "$BuildRoot/panel-interaction-test.exe"
    if ($LASTEXITCODE) { throw 'Panel interaction tests failed' }
    & $zigCommand c++ "$root/tests/badge_style_test.cpp" "$root/src/folder_badge.cpp" -std=c++17 -O2 -UNDEBUG -municode -o "$BuildRoot/badge-style-test.exe" -luser32 -lgdi32 -lgdiplus -lshell32 -lole32 -luuid -lshlwapi
    if ($LASTEXITCODE) { throw 'Badge style test compilation failed' }
    & "$BuildRoot/badge-style-test.exe" (Join-Path $BuildRoot ("badge-style-" + [guid]::NewGuid().ToString('N')))
    if ($LASTEXITCODE) { throw 'Badge style tests failed' }
    # Lifecycle safety tests must be part of the normal build, not only local candidate scripts.
    $entryService = @("$root/src/entry_maintenance.cpp", "$root/src/entry_actions.cpp", "$root/src/config_store.cpp", "$root/src/shell.cpp", "$root/src/folder_rules.cpp", "$root/src/folder_badge.cpp", "$root/src/context_menu.cpp")
    $entryLibs = @('-luser32','-lshell32','-lgdi32','-lgdiplus','-lcomctl32','-lole32','-loleaut32','-loleacc','-luuid','-ladvapi32','-lshlwapi')
    foreach ($test in @('entry_maintenance','desktop_scope','badge_removal')) {
        $binary = Join-Path $BuildRoot ($test + '-test.exe')
        & $zigCommand c++ "$root/tests/$test`_test.cpp" @entryService -std=c++17 -O2 -UNDEBUG -municode -o $binary @entryLibs
        if ($LASTEXITCODE) { throw "$test test compilation failed" }
        $modes = if ($test -eq 'entry_maintenance') { @('default','badge','watch') } else { @('default') }
        foreach ($mode in $modes) {
            $fixture = Join-Path $BuildRoot ($test + '-' + $mode + '-' + [guid]::NewGuid().ToString('N'))
            if ($mode -eq 'default') { & $binary $fixture } else { & $binary $fixture $mode }
            if ($LASTEXITCODE) { throw "$test $mode tests failed" }
        }
    }
    if ($TestsOnly) { return }
    & $zigCommand rc /fo "$BuildRoot/app.res" "$root/src/app.rc"
    if ($LASTEXITCODE) { throw 'Icon resource compilation failed' }
    & $zigCommand c++ "$BuildRoot/app.res" "$root/src/entry_maintenance.cpp" "$root/src/entry_actions.cpp" "$root/src/main.cpp" "$root/src/shell.cpp" "$root/src/config_store.cpp" "$root/src/panel.cpp" "$root/src/settings.cpp" "$root/src/icon_cache.cpp" "$root/src/folder_sort.cpp" "$root/src/folder_rules.cpp" "$root/src/sort_settings.cpp" "$root/src/folder_badge.cpp" "$root/src/context_menu.cpp" -std=c++17 -Os -s -municode '-Wl,--subsystem,windows' -o "$BuildRoot/FolderIconCN.exe" -luser32 -lshell32 -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -loleaut32 -loleacc -luuid -ladvapi32 -ldwmapi -lshlwapi -lgdiplus -lbcrypt
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
