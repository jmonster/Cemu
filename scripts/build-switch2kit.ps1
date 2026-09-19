param([switch]$Run)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Set-Location (Join-Path $PSScriptRoot '..')
if (-not [Environment]::Is64BitOperatingSystem -or -not [Environment]::Is64BitProcess) {
    throw 'Use 64-bit PowerShell on x64 Windows.'
}
foreach ($tool in @('git', 'cmake', 'ninja', 'swift', 'swiftc')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing build tool: $tool" }
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw 'Install Visual Studio 2022 Desktop development with C++ and a Windows SDK.' }
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual C++ x64 tools were not found.' }
cmd /c "`"$vs\Common7\Tools\VsDevCmd.bat`" -arch=x64 -host_arch=x64 >nul && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the Visual C++ environment.' }
$target = swiftc -print-target-info | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $target.target.triple -notmatch '^x86_64-.*windows-msvc$') {
    throw 'Install the native x64 Swift toolchain; ARM64 and cross-compilation are not supported here.'
}
# Keep runtime lookup local to this process and its launched application.
$env:PATH = (($target.paths.runtimeLibraryPaths | Where-Object { Test-Path $_ }) -join ';') + ';' + $env:PATH
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not (Test-Path 'dependencies/vcpkg/vcpkg.exe')) {
    & .\dependencies\vcpkg\bootstrap-vcpkg.bat -disableMetrics
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not $env:VCPKG_MAX_CONCURRENCY) { $env:VCPKG_MAX_CONCURRENCY = '3' }
cmake -S . -B build-switch2kit-windows -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DENABLE_SWITCH2KIT=ON -DENABLE_SDL=ON -DENABLE_VULKAN=ON -DMACOS_BUNDLE=OFF
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build-switch2kit-windows --target CemuBin --parallel 3
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$app = Join-Path $PWD 'bin/Cemu_release.exe'
if (-not (Test-Path $app) -or -not (Test-Path (Join-Path (Split-Path $app) 'Switch2KitC.dll'))) {
    throw 'The controller-enabled application or its native DLL is missing.'
}
Write-Host "Built: $app"
if ($Run) { Start-Process -FilePath $app -WorkingDirectory (Split-Path $app) }
