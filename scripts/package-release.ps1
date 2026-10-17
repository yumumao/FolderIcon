[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidatePattern('^v2\.\d+\.\d+(?:-rc\.\d+)?$')][string]$Version,
    [Parameter(Mandatory)][string]$Exe,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$Exe=(Resolve-Path -LiteralPath $Exe).Path
if((Split-Path $Exe -Leaf) -cne 'FolderIconCN.exe'){throw 'Expected FolderIconCN.exe, not the legacy upstream executable'}
$bytes=[IO.File]::ReadAllBytes($Exe)
if($bytes.Length-lt256-or $bytes[0]-ne0x4d-or $bytes[1]-ne0x5a){throw 'Not a PE executable'}
$pe=[BitConverter]::ToInt32($bytes,60)
if($pe-lt0-or $pe+6-gt$bytes.Length-or [BitConverter]::ToUInt32($bytes,$pe)-ne0x4550-or [BitConverter]::ToUInt16($bytes,$pe+4)-ne0x8664){throw 'Expected a Windows x64 PE executable'}
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$name="FolderIconCN-$Version-win-x64"
$stage=Join-Path $OutputDirectory $name
foreach($destination in @($stage,(Join-Path $OutputDirectory "$name.zip"),(Join-Path $OutputDirectory 'FolderIconCN.exe'),(Join-Path $OutputDirectory 'SHA256SUMS.txt'))){
    if(Test-Path -LiteralPath $destination){throw 'Packaging destination already exists; use a fresh output directory'}
}
$public=@('README.md','README.en.md','config/FolderIcon.example.ini','docs/PRIVACY.md','docs/UPSTREAM.md','docs/VALIDATION.md','docs/DESKTOP-MODES.md','docs/RELEASE-v2.0.0-rc.1.md','docs/RELEASE-v2.0.0-rc.2.md',"docs/RELEASE-$Version.md",'docs/images/panel.png','docs/images/subfolder.png','docs/images/settings-appearance.png','docs/images/settings-entries.png','docs/images/real-desktop-orange.png','docs/images/real-shortcut-blue.png','docs/images/hero-click-expand.png','docs/images/exit-confirmation.png')
$public=@($public|Select-Object -Unique)
foreach($relative in $public){if(!(Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)){throw "Missing public input: $relative"}}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath $Exe -Destination "$stage/FolderIconCN.exe"
foreach($relative in $public){$dest=Join-Path $stage $relative;New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null;Copy-Item -LiteralPath (Join-Path $root $relative) -Destination $dest}
$zip=Join-Path $OutputDirectory "$name.zip"
if(Test-Path -LiteralPath $zip){throw 'Archive already exists; refusing to overwrite'}
Compress-Archive -LiteralPath $stage -DestinationPath $zip -CompressionLevel Optimal
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead($zip)
try{
    $actual=@($archive.Entries | Where-Object {$_.Name} | ForEach-Object {$_.FullName.Replace('\','/')})
    $expected=@('FolderIconCN.exe')+$public | ForEach-Object {"$name/$_"}
    if(@(Compare-Object ($expected|Sort-Object) ($actual|Sort-Object)).Count){throw 'ZIP content differs from the explicit public allowlist'}
}finally{$archive.Dispose()}
Copy-Item -LiteralPath $Exe -Destination "$OutputDirectory/FolderIconCN.exe"
$lines=foreach($file in @('FolderIconCN.exe',"$name.zip")){'{0}  {1}' -f (Get-FileHash -LiteralPath (Join-Path $OutputDirectory $file) -Algorithm SHA256).Hash.ToLowerInvariant(),$file}
[IO.File]::WriteAllText("$OutputDirectory/SHA256SUMS.txt",($lines -join "`r`n")+"`r`n",[Text.UTF8Encoding]::new($false))
[pscustomobject]@{Version=$Version;Zip=$zip;Files=$actual.Count;ExeSHA256=(Get-FileHash -LiteralPath $Exe -Algorithm SHA256).Hash;Checksums="$OutputDirectory/SHA256SUMS.txt"}
