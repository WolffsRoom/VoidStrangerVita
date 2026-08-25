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
$internationalWin = Join-Path $root 'data\builder-work\international\data.win'
$selectedDataWin = Join-Path $SourceRoot 'data.win'
if (-not $OriginalData) {
  if (Test-Path -LiteralPath $optimizedWin) { $selectedDataWin = $optimizedWin }
  elseif (Test-Path -LiteralPath $internationalWin) { $selectedDataWin = $internationalWin }
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
foreach ($name in $required) { Copy-Item -LiteralPath (Join-Path $SourceRoot $name) -Destination (Join-Path $OutputRoot $name) -Force }
foreach ($name in @('options.ini', 'splash.png')) {
  $path = Join-Path $SourceRoot $name
  if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination (Join-Path $OutputRoot $name) -Force }
}
$languages = Join-Path $root 'mods\Languages'
if (-not $OriginalData -and (Test-Path -LiteralPath $languages)) {
  Copy-Item -LiteralPath $languages -Destination $OutputRoot -Recurse -Force
}
$cacheRoot = Join-Path $root 'data\voidstranger_builder\prepare_texture_cache_v0_3_7\prepared\chapter1'
foreach ($folder in @('texture-cache', 'pvr')) {
  $cache = Join-Path $cacheRoot $folder
  if (-not $OriginalData -and (Test-Path -LiteralPath $cache)) {
    Copy-Item -LiteralPath $cache -Destination $OutputRoot -Recurse -Force
  }
}
Get-ChildItem -LiteralPath $OutputRoot -Recurse -File | Get-FileHash -Algorithm SHA256 |
  ForEach-Object { "{0}  {1}" -f $_.Hash, $_.Path.Substring($OutputRoot.Length + 1) } |
  Set-Content -LiteralPath (Join-Path $OutputRoot 'manifest-sha256.txt') -Encoding ascii
Write-Host "Prepared data: $OutputRoot"
Write-Host "data.win source: $selectedDataWin"
Write-Host 'Copy the voidstranger folder to ux0:data/ on the PS Vita.'
