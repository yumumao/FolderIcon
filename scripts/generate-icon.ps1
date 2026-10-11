# Original geometric folder/grid artwork. Does not use any user's desktop icons.
[CmdletBinding()]
param([string]$OutputPath=(Join-Path (Split-Path $PSScriptRoot -Parent) 'assets/app.ico'))
Add-Type -AssemblyName System.Drawing
function RoundRect($g,$brush,[single]$x,[single]$y,[single]$w,[single]$h,[single]$r){
 $p=[Drawing.Drawing2D.GraphicsPath]::new();$d=2*$r
 $p.AddArc($x,$y,$d,$d,180,90);$p.AddArc($x+$w-$d,$y,$d,$d,270,90)
 $p.AddArc($x+$w-$d,$y+$h-$d,$d,$d,0,90);$p.AddArc($x,$y+$h-$d,$d,$d,90,90)
 $p.CloseFigure();$g.FillPath($brush,$p);$p.Dispose()
}
$frames=@();$sizes=@(16,20,24,32,40,48,64,128,256)
foreach($size in $sizes){
 $b=[Drawing.Bitmap]::new($size,$size);$g=[Drawing.Graphics]::FromImage($b)
 $g.SmoothingMode='AntiAlias';$g.Clear([Drawing.Color]::Transparent);$g.ScaleTransform($size/256.0,$size/256.0)
 $navy=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255,29,52,80))
 $tab=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255,244,174,49))
 $gold=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255,255,204,86))
 $white=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255,255,251,237))
 RoundRect $g $navy 8 8 240 240 52
 RoundRect $g $tab 35 57 91 57 12;RoundRect $g $tab 35 79 186 123 14
 RoundRect $g $gold 29 91 198 117 15
 foreach($x in @(65,112,159)){foreach($y in @(116,159)){RoundRect $g $white $x $y 30 28 6}}
 $g.Dispose();$navy.Dispose();$tab.Dispose();$gold.Dispose();$white.Dispose()
 $ms=[IO.MemoryStream]::new();$b.Save($ms,[Drawing.Imaging.ImageFormat]::Png);$frames+=,$ms.ToArray();$ms.Dispose();$b.Dispose()
}
$dir=Split-Path $OutputPath -Parent;New-Item -ItemType Directory -Path $dir -Force|Out-Null
$stream=[IO.File]::Create($OutputPath);$writer=[IO.BinaryWriter]::new($stream)
try{
 $writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$sizes.Count);$offset=6+16*$sizes.Count
 for($i=0;$i -lt $sizes.Count;$i++){$s=if($sizes[$i]-eq 256){0}else{$sizes[$i]};$writer.Write([byte]$s);$writer.Write([byte]$s);$writer.Write([byte]0);$writer.Write([byte]0);$writer.Write([uint16]1);$writer.Write([uint16]32);$writer.Write([uint32]$frames[$i].Length);$writer.Write([uint32]$offset);$offset+=$frames[$i].Length}
 foreach($frame in $frames){$writer.Write([byte[]]$frame)}
}finally{$writer.Dispose();$stream.Dispose()}
Write-Output "Created original multi-resolution app icon: $($sizes -join ', ')px"
