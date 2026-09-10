param(
  [string]$SourceRoot,
  [string]$OutputRoot,
  [switch]$Force,
  [switch]$OriginalData
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $SourceRoot) { $SourceRoot = Join-Path $root 'data\Void.Stranger.v1.1.3' }
if (-not $OutputRoot) { $OutputRoot = Join-Path $root 'data\prepared\voidstranger' }
$optimizedWin = Join-Path $root 'data\voidstranger_builder\rebuild_data_win_v1_6\rebuild\voidstranger\data.win'
$performanceV4Win = Join-Path $root 'data\builder-work\performance-v4\data.win'
$optimizedAudioRoot = Join-Path $root 'data\voidstranger_builder\optimized_audio_1_1_3'
$selectedDataWin = Join-Path $SourceRoot 'data.win'
if (-not $OriginalData) {
  if (Test-Path -LiteralPath $performanceV4Win) { $selectedDataWin = $performanceV4Win }
  elseif (Test-Path -LiteralPath $optimizedWin) { $selectedDataWin = $optimizedWin }
}
$required = @('audiogroup1.dat', 'audiogroup2.dat', 'voidstranger_data.csv')
foreach ($name in $required) {
  $path = Join-Path $SourceRoot $name
  if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing required file: $path" }
}
if (-not (Test-Path -LiteralPath $selectedDataWin -PathType Leaf)) { throw "Missing data.win: $selectedDataWin" }
if ((Test-Path -LiteralPath $OutputRoot) -and -not $Force) {
  throw "Output already exists: $OutputRoot (use -Force to refresh it)"
}
if (Test-Path -LiteralPath $OutputRoot) {
  # Recreate the exact deployment tree so obsolete Deltarune-style folders
  # such as chapter1/, music/, sounds/, and ui/ cannot survive a refresh.
  Remove-Item -LiteralPath $OutputRoot -Recurse -Force
}
New-Item -ItemType Directory -Force $OutputRoot | Out-Null
Copy-Item -LiteralPath $selectedDataWin -Destination (Join-Path $OutputRoot 'data.win') -Force
foreach ($name in $required) {
  $source = Join-Path $SourceRoot $name
  if (-not $OriginalData -and $name -like 'audiogroup*.dat') {
    $optimized = Join-Path $optimizedAudioRoot $name
    if (Test-Path -LiteralPath $optimized -PathType Leaf) { $source = $optimized }
  }
  Copy-Item -LiteralPath $source -Destination (Join-Path $OutputRoot $name) -Force
}
foreach ($name in @('options.ini', 'splash.png', 'csvstring.ini', 'trophies.ini', 'config.ini', 'dbgconf.ini', 'settings.vs', 'save.vs', 'settings.vslocal')) {
  $path = Join-Path $SourceRoot $name
  if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination (Join-Path $OutputRoot $name) -Force }
}
$savesDir = Join-Path $OutputRoot 'saves'
New-Item -ItemType Directory -Force $savesDir | Out-Null
foreach ($name in @('trophies.ini', 'config.ini', 'dbgconf.ini', 'settings.vs', 'save.vs', 'settings.vslocal', 'csvstring.ini')) {
  $path = Join-Path $OutputRoot $name
  if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination (Join-Path $savesDir $name) -Force }
}
$languages = Join-Path $root 'mods\Languages'
if (-not $OriginalData -and (Test-Path -LiteralPath $languages)) {
  Copy-Item -LiteralPath $languages -Destination $OutputRoot -Recurse -Force
}
# Texture caches are intentionally NOT copied from the PC build.
# On Vita, the active data.win is decoded once and the runtime creates:
#   texture-cache/*.r444 for lossless/UI pages
#   pvr/*.bc3.pvr for BC3-eligible pages
# Cache fingerprints/magic invalidate stale files after a data.win rebuild.
Get-ChildItem -LiteralPath $OutputRoot -Recurse -File | Get-FileHash -Algorithm SHA256 |
  ForEach-Object { "{0}  {1}" -f $_.Hash, $_.Path.Substring($OutputRoot.Length + 1) } |
  Set-Content -LiteralPath (Join-Path $OutputRoot 'manifest-sha256.txt') -Encoding ascii
Write-Host "Prepared data: $OutputRoot"
Write-Host "data.win source: $selectedDataWin"
Write-Host 'Copy the voidstranger folder to ux0:data/ on the PS Vita.'



