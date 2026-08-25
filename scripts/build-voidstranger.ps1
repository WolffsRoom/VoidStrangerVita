param([ValidateSet('Debug','Release')][string]$Configuration = 'Release', [switch]$CleanVitaGL)
$ErrorActionPreference = 'Stop'
$rootPath = Split-Path -Parent $PSScriptRoot
$mount = $rootPath.Replace('\', '/')
$clean = if ($CleanVitaGL) { 'make -C /project/third_party/vitaGL-nosplash clean && ' } else { '' }
docker run --rm -v "${mount}:/project" -w /project/src/vita-runner `
  atamanenko/vitasdk-softfp:latest sh -lc `
  "${clean}make -C /project/third_party/vitaGL-nosplash -j2 SOFTFP_ABI=1 NO_SPLASHSCREEN=1 NO_DEBUG=1 SINGLE_THREADED_GC=1 && cmake -S . -B build-vita -DCMAKE_BUILD_TYPE=$Configuration && cmake --build build-vita -j2"
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE." }
$out = Join-Path $rootPath 'artifacts\VoidStranger.vpk'
New-Item -ItemType Directory -Force (Split-Path -Parent $out) | Out-Null
Copy-Item -Force (Join-Path $rootPath 'src\vita-runner\build-vita\VoidStranger.vpk') $out
Write-Host "VPK: $out"
