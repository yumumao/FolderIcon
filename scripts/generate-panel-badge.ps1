[CmdletBinding()]
param([Parameter(Mandatory)][string]$InputIco,[Parameter(Mandatory)][string]$OutputIco)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$inputPath=[IO.Path]::GetFullPath($InputIco);$outputPath=[IO.Path]::GetFullPath($OutputIco)
if($inputPath.Equals($outputPath,[StringComparison]::OrdinalIgnoreCase)){throw 'Input and output must differ; the original icon is never edited.'}
if(Test-Path -LiteralPath $outputPath){throw 'Output already exists; choose a new candidate file.'}
$source=[IO.File]::ReadAllBytes($inputPath);$originalHash=(Get-FileHash -LiteralPath $inputPath).Hash
if($source.Length-lt6-or[BitConverter]::ToUInt16($source,0)-ne0-or[BitConverter]::ToUInt16($source,2)-ne1){throw 'Not an ICO file'}
$count=[BitConverter]::ToUInt16($source,4);if(!$count-or$count-gt256-or$source.Length-lt(6+16*$count)){throw 'Invalid ICO directory'}
$frames=[Collections.Generic.List[object]]::new()
for($i=0;$i-lt$count;$i++){
 $at=6+16*$i;$size=if($source[$at]){[int]$source[$at]}else{256};$height=if($source[$at+1]){[int]$source[$at+1]}else{256};$length=[BitConverter]::ToUInt32($source,$at+8);$offset=[BitConverter]::ToUInt32($source,$at+12)
 if($size-ne$height-or$length-lt8-or([long]$offset+$length)-gt$source.Length){throw 'Invalid or non-square ICO frame'}
 if([BitConverter]::ToString($source,$offset,8)-ne'89-50-4E-47-0D-0A-1A-0A'){throw 'This offline generator accepts PNG-compressed ICO frames only.'}
 $stream=[IO.MemoryStream]::new($source,[int]$offset,[int]$length,$false);$image=[Drawing.Bitmap]::FromStream($stream)
 $bitmap=$null;$g=$null;$brush=$null;$white=$null;$png=$null
 try{
  if($image.Width-ne$size-or$image.Height-ne$height){throw 'PNG dimensions do not match ICO directory'}
  $bitmap=$image.Clone([Drawing.Rectangle]::new(0,0,$size,$size),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $g=[Drawing.Graphics]::FromImage($bitmap);$g.SmoothingMode='AntiAlias';$g.PixelOffsetMode='Half'
  # Matches src/badge_visual.h. Only the frame shrinks; tile/gap use approved 36%.
  $d=[Math]::Max(8,[int][Math]::Floor($size*0.32));$reference=[Math]::Max(8,[int][Math]::Floor($size*0.36));$x=$size-$d
  $g.SetClip([Drawing.Rectangle]::new($x,0,$d,$d));$g.SmoothingMode='AntiAlias';$g.PixelOffsetMode='Half'
  $white=[Drawing.SolidBrush]::new([Drawing.Color]::White);$brush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255,29,106,177))
  $stroke=[Math]::Max(1,[int][Math]::Floor($d/20));$radius=[Math]::Max(2,[int][Math]::Floor(($d*23+50)/100))
  function RoundedPath([single]$rx,[single]$ry,[single]$extent,[single]$radius){
   $path=[Drawing.Drawing2D.GraphicsPath]::new();$a=2*$radius
   $path.AddArc($rx,$ry,$a,$a,180,90);$path.AddArc($rx+$extent-$a,$ry,$a,$a,270,90)
   $path.AddArc($rx+$extent-$a,$ry+$extent-$a,$a,$a,0,90);$path.AddArc($rx,$ry+$extent-$a,$a,$a,90,90);$path.CloseFigure();return ,$path
  }
  $outer=RoundedPath $x 0 $d $radius;$inner=RoundedPath ($x+$stroke) $stroke ($d-2*$stroke) ([Math]::Max(1,$radius-$stroke))
  try{$g.FillPath($white,$outer);$g.FillPath($brush,$inner)}finally{$outer.Dispose();$inner.Dispose()}
  $tile=[Math]::Max(1,[int][Math]::Floor($reference*21/100));$gap=[Math]::Max(1,[int][Math]::Floor($reference*11/100));$left=[int][Math]::Floor(($d-2*$tile-$gap)/2)
  $g.SmoothingMode='None'
  for($row=0;$row-lt2;$row++){for($col=0;$col-lt2;$col++){$g.FillRectangle($white,[single]($x+$left+$col*($tile+$gap)),[single]($left+$row*($tile+$gap)),[single]$tile,[single]$tile)}}
  # The lower-left shortcut arrow region and all pixels outside the badge stay untouched.
  for($py=0;$py-lt$size;$py++){for($px=0;$px-lt$size;$px++){if($px-ge$x-and$py-lt$d){continue};if($image.GetPixel($px,$py).ToArgb()-ne$bitmap.GetPixel($px,$py).ToArgb()){throw "Pixel outside badge changed: frame=$size x=$px y=$py"}}}
  $png=[IO.MemoryStream]::new();$bitmap.Save($png,[Drawing.Imaging.ImageFormat]::Png);$frames.Add(@{Size=$size;Data=$png.ToArray()})
 }finally{if($png){$png.Dispose()};if($white){$white.Dispose()};if($brush){$brush.Dispose()};if($g){$g.Dispose()};if($bitmap){$bitmap.Dispose()};$image.Dispose();$stream.Dispose()}
}
$parent=[IO.Path]::GetDirectoryName($outputPath);if(!(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
$file=[IO.File]::Open($outputPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None);$writer=[IO.BinaryWriter]::new($file)
try{
 $writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$count);$offset=6+16*$count
 foreach($frame in $frames){$dimension=if($frame.Size-eq256){0}else{$frame.Size};$writer.Write([byte]$dimension);$writer.Write([byte]$dimension);$writer.Write([byte]0);$writer.Write([byte]0);$writer.Write([uint16]1);$writer.Write([uint16]32);$writer.Write([uint32]$frame.Data.Length);$writer.Write([uint32]$offset);$offset+=$frame.Data.Length}
 foreach($frame in $frames){$writer.Write([byte[]]$frame.Data)}
}finally{$writer.Dispose()}
if((Get-FileHash -LiteralPath $inputPath).Hash-ne$originalHash){throw 'Original icon integrity check failed'}
[pscustomobject]@{Output=$outputPath;Frames=$count;Sizes=@($frames|ForEach-Object Size);OriginalUnchanged=$true;OutsideBadgeUnchanged=$true;SHA256=(Get-FileHash -LiteralPath $outputPath).Hash}
