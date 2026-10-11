[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    # Review this allowlist whenever a new public file is introduced.
    $allowed = @(
        '.gitattributes', '.gitignore', '.github/workflows/build-release.yml',
        'README.md', 'README.en.md', 'CMakeLists.txt', 'main.c', 'FolderIcon.sln',
        'FolderIconC.vcxproj', 'build.bat', 'build_simple.cmd',
        'src/app.h', 'src/core.h', 'src/main.cpp', 'src/shell.cpp',
        'src/panel.cpp', 'src/settings.cpp', 'src/app.rc', 'assets/app.ico',
        'scripts/generate-icon.ps1', 'tests/core_test.cpp',
        'src/icon_cache.h', 'src/icon_cache.cpp', 'tests/icon_cache_test.cpp',
        'src/folder_sort.h', 'src/folder_sort.cpp', 'src/sort_settings.h', 'src/sort_settings.cpp', 'tests/folder_sort_test.cpp',
        'src/folder_rules.h', 'src/folder_rules.cpp', 'tests/folder_rules_test.cpp',
        'src/folder_navigation.h', 'tests/folder_navigation_test.cpp',
        'src/config_store.cpp', 'tests/config_store_test.cpp',
        'src/entry_actions.h', 'src/entry_actions.cpp', 'tests/entry_actions_test.cpp',
        'src/folder_badge.h', 'src/folder_badge.cpp', 'tests/folder_badge_test.cpp',
        'scripts/generate-panel-badge.ps1',
        'src/badge_visual.h', 'tests/badge_style_test.cpp',
        'src/panel_interaction.h', 'tests/panel_interaction_test.cpp',
        'src/context_menu.h', 'src/context_menu.cpp', 'tests/context_menu_test.cpp',
        'src/desktop_scope.h', 'tests/desktop_scope_test.cpp', 'tests/badge_removal_test.cpp',
        'src/entry_maintenance.h', 'src/entry_maintenance.cpp', 'tests/entry_maintenance_test.cpp',
        'scripts/build.ps1', 'scripts/check-publish.ps1', 'scripts/package-release.ps1',
        'docs/RELEASE-v2.0.0-rc.1.md',
        'docs/images/panel.png', 'docs/images/subfolder.png',
        'docs/images/settings-appearance.png', 'docs/images/settings-entries.png', 'docs/images/real-desktop-orange.png', 'docs/images/real-shortcut-blue.png',
        'config/FolderIcon.example.ini', 'docs/PRIVACY.md',
        'docs/UPSTREAM.md', 'docs/VALIDATION.md', 'docs/dark.png', 'docs/light.png'
    )
    $files = @(git -c core.quotepath=false ls-files)
    if ($LASTEXITCODE) { throw 'Cannot read Git index' }
    $errors = [Collections.Generic.List[string]]::new()
    $patterns = [ordered]@{
        'personal absolute path' = '(?i)[A-Z]:[\\/]+(?:Users|work|ls|Dropbox|SYS_Downloads)[\\/]'
        'GitHub token' = '(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,})'
        'private key' = ('-----BEGIN ' + '(?:RSA |EC |OPENSSH )?PRIVATE KEY-----')
        'API key' = '\bsk-[A-Za-z0-9_-]{20,}'
        'credential in URL' = 'https?://[^/\s:@]+:[^/\s@]+@'
    }
    $localNames = @($env:USERNAME, $env:COMPUTERNAME) | Where-Object { $_ -and $_.Length -ge 5 }
    foreach ($file in $files) {
        if ($file -notin $allowed) { $errors.Add("Unapproved tracked path: $file"); continue }
        $size = git cat-file -s ":$file"
        if ($LASTEXITCODE) { throw "Cannot read index blob: $file" }
        if ([long]$size -gt 2097152) { $errors.Add("Oversized file: $file") }
        # Only explicitly reviewed public screenshots and the application icon may be binary.
        if ($file -in @('docs/dark.png', 'docs/light.png', 'assets/app.ico', 'docs/images/panel.png', 'docs/images/subfolder.png', 'docs/images/settings-appearance.png', 'docs/images/settings-entries.png', 'docs/images/real-desktop-orange.png', 'docs/images/real-shortcut-blue.png')) { continue }
        $content = (git show ":$file") -join "`n"
        if ($LASTEXITCODE) { throw "Cannot read staged content: $file" }
        foreach ($item in $patterns.GetEnumerator()) {
            if ($content -match $item.Value) { $errors.Add("$file : $($item.Key)") }
        }
        foreach ($localName in $localNames) {
            if ($content.IndexOf($localName, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
                $errors.Add("$file : local user/machine identifier")
            }
        }
        if ($content.Contains([char]0)) { $errors.Add("Unexpected binary text file: $file") }
    }
    # Verify ignore rules even when local artifacts do not exist in a fresh checkout.
    $probes = @('dist/FolderIcon.ini', 'dist/icons/private.ico', '.codex/work.log',
                '.claude/settings.json', '.shared/context.md', '.env', 'private-note.txt',
                'src/test-secret.pem', 'docs/desktop.png', 'config/FolderIcon.ini',
                'README.md.bak', 'upstream/main.c', 'assets/private.ico', 'docs/images/private-desktop.png',
                'cache/icons-v1/local.fic', 'src/cache/icons-v1/local.fic', 'layout/personal.order', 'src/layout/private.order')
    foreach ($probe in $probes) {
        git check-ignore -q --no-index -- $probe
        if ($LASTEXITCODE -ne 0) { $errors.Add("Not ignored: $probe") }
    }
    git diff --cached --check
    if ($LASTEXITCODE) { $errors.Add('Staged whitespace check failed') }
    if ($errors.Count) {
        foreach ($errorMessage in $errors) { Write-Output $errorMessage }
        throw "Publish check FAILED ($($errors.Count) findings); no content values were printed."
    }
    Write-Output "Publish check passed: $($files.Count) tracked files; $($probes.Count) ignored-path probes."
} finally {
    Pop-Location
}
