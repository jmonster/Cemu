param([switch]$Run)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Set-Location (Join-Path $PSScriptRoot '..')
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or
    -not [Environment]::Is64BitOperatingSystem -or -not [Environment]::Is64BitProcess) {
    throw 'Use 64-bit PowerShell on x64 Windows.'
}
foreach ($tool in @('git', 'cmake', 'ninja', 'swift')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing build tool: $tool" }
}
# Resolve one Swift installation before VsDevCmd changes PATH. Reuse these
# exact executables when reconfiguring an existing CMake build directory.
$swift = (Get-Command swift -CommandType Application).Source
$swiftc = Join-Path (Split-Path $swift) 'swiftc.exe'
if (-not (Test-Path $swiftc)) { throw 'The selected Swift installation has no swiftc.exe.' }
$swiftVersion = & $swift --version
if ($LASTEXITCODE -ne 0) { throw 'Could not query the Swift toolchain version.' }
Write-Output ($swiftVersion -join "`n")
if (($swiftVersion -join "`n") -notmatch 'Swift version 6\.2\.1(?:[ (]|$)') {
    throw 'This helper requires native x64 Swift 6.2.1 with Visual Studio 2022. Other toolchain combinations are not qualified.'
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw 'Install Visual Studio 2022 Desktop development with C++ and a Windows SDK.' }
$vs = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vs) { throw 'Install Visual Studio 2022 Desktop development with C++ and a Windows SDK. Visual Studio 2026 is not compatible with this Swift toolchain.' }
cmd /c "`"$vs\Common7\Tools\VsDevCmd.bat`" -arch=x64 -host_arch=x64 >nul && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the Visual C++ environment.' }
$targetInfo = & $swiftc -print-target-info
if ($LASTEXITCODE -ne 0) { throw 'Could not query the Swift target.' }
$target = $targetInfo | ConvertFrom-Json
if ($target.target.triple -notmatch '^x86_64-.*windows-msvc$') {
    throw 'Install the native x64 Swift toolchain; ARM64 and cross-compilation are not supported here.'
}
Write-Output "Visual Studio: $vs"
Write-Output "Swift target: $($target.target.triple)"
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
    -DENABLE_SWITCH2KIT=ON -DENABLE_SDL=ON -DENABLE_VULKAN=ON -DMACOS_BUNDLE=OFF `
    "-DSWITCH2KIT_SWIFT=$swift" "-DSWITCH2KIT_SWIFTC=$swiftc"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build-switch2kit-windows --target CemuBin --parallel 3
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$app = Join-Path $PWD 'bin/Cemu_release.exe'
if (-not (Test-Path $app) -or -not (Test-Path (Join-Path (Split-Path $app) 'Switch2KitC.dll'))) {
    throw 'The controller-enabled application or its native DLL is missing.'
}
Write-Host "Built: $app"
if ($Run) { Start-Process -FilePath $app -WorkingDirectory (Split-Path $app) }
