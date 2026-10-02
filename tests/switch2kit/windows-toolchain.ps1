# Manual, dependency-free PowerShell regression for the Windows build helper.
# It runs the production statements with native process/filesystem boundaries
# mocked. It does not compile Cemu or qualify a Windows application package.
param([string]$HelperPath = (Join-Path $PSScriptRoot '../../scripts/build-switch2kit.ps1'))
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$tokens = $null
$errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    $HelperPath, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
# The host guard must remain in production. Omit only that guard to execute the
# same toolchain/build statements on a non-Windows PowerShell test host.
$statements = @($ast.EndBlock.Statements)
$guards = @($statements | Where-Object {
    $_ -is [System.Management.Automation.Language.IfStatementAst] -and
    $_.Extent.Text.Contains('[Environment]::OSVersion.Platform')
})
if ($guards.Count -ne 1 -or -not $guards[0].Extent.Text.Contains('Is64BitProcess')) {
    throw 'Expected the Windows/x64 production precondition.'
}
$body = [scriptblock]::Create('param($PSScriptRoot, [switch]$Run)' + "`n" + ((($statements | Where-Object { $_ -ne $guards[0] }).Extent.Text) -join "`n"))
$originalPath = $env:PATH
$originalProgramFiles = ${env:ProgramFiles(x86)}
try {
    ${env:ProgramFiles(x86)} = '/fake-program-files'
    $script:swift = Join-Path $PSScriptRoot 'fake Swift/bin/swift.exe'
    $script:swiftc = Join-Path (Split-Path $script:swift) 'swiftc.exe'
    Set-Alias -Name $script:swift -Value Invoke-FakeSwift -Scope Script
    Set-Alias -Name $script:swiftc -Value Invoke-FakeSwiftc -Scope Script
    function Set-Location { param($Path) }
    function Get-Command {
        param($Name, $CommandType, $ErrorAction)
        if ($Name -eq 'swift') { return [pscustomobject]@{Source = $script:swift} }
        return [pscustomobject]@{Source = $Name}
    }
    function Test-Path {
        param($Path)
        if ($Path -eq $script:swiftc) { return -not $script:missingSibling }
        if ($Path -match 'vswhere\.exe$') {
            Set-Alias -Name $Path -Value Invoke-FakeVsWhere -Scope Script
        }
        return $true
    }
    function Invoke-FakeSwift {
        $global:LASTEXITCODE = $script:swiftExit
        return $script:version
    }
    function Invoke-FakeSwiftc {
        $global:LASTEXITCODE = $script:targetExit
        return (@{target = @{triple = $script:triple}; paths = @{runtimeLibraryPaths = @('/fake-runtime')}} | ConvertTo-Json)
    }
    function Invoke-FakeVsWhere {
        $script:vsArguments = @($args)
        $global:LASTEXITCODE = $script:vsExit
        # Simulate VS 2022 and VS 2026 installed together. Unbounded -latest
        # selects the incompatible 2026 installation.
        if ($script:noVS) { return }
        if ($args -contains '[17.0,18.0)') { return '/VS2022' }
        return '/VS2026'
    }
    function cmd {
        if (($args -join ' ') -notmatch '/VS2022') { throw 'Wrong Visual Studio selected' }
        $global:LASTEXITCODE = 0
        # Simulate VsDevCmd prioritizing a different Swift on PATH.
        return 'PATH=/different-swift-bin'
    }
    function git { $global:LASTEXITCODE = 0 }
    function cmake {
        $script:cmakeCalls += ,@($args)
        $global:LASTEXITCODE = 0
    }
    function Start-Process { throw 'The test must never launch an application' }
    function Reset-Scenario {
        $script:missingSibling = $false
        $script:version = 'Swift version 6.2.1 (swift-6.2.1-RELEASE)'
        $script:swiftExit = 0
        $script:targetExit = 0
        $script:vsExit = 0
        $script:noVS = $false
        $script:triple = 'x86_64-unknown-windows-msvc'
        $script:cmakeCalls = @()
        $script:vsArguments = @()
        $env:PATH = $originalPath
    }
    function Expect-Rejection {
        param([string]$Expected)
        $failure = ''
        try { & $body /fake-root | Out-Null } catch { $failure = $_.Exception.Message }
        if ($failure -notlike "*$Expected*") { throw "Expected rejection '$Expected', got '$failure'" }
        if ($script:cmakeCalls.Count) { throw 'Rejected toolchain reached CMake' }
    }
    Reset-Scenario
    & $body /fake-root | Out-Null
    if ($script:cmakeCalls.Count -ne 2) { throw 'Expected configure and build' }
    $configure = $script:cmakeCalls[0]
    foreach ($argument in @("-DSWITCH2KIT_SWIFT=$script:swift", "-DSWITCH2KIT_SWIFTC=$script:swiftc")) {
        if ($configure -notcontains $argument) { throw "CMake did not receive exact selected tool: $argument" }
    }
    foreach ($candidateVersion in @('Swift version 6.2.10', 'Swift version 6.3.3', 'unrecognized version')) {
        Reset-Scenario
        $script:version = $candidateVersion
        Expect-Rejection 'requires native x64 Swift 6.2.1'
    }
    Reset-Scenario; $script:missingSibling = $true; Expect-Rejection 'no swiftc.exe'
    Reset-Scenario; $script:swiftExit = 1; Expect-Rejection 'query the Swift toolchain version'
    Reset-Scenario; $script:noVS = $true; Expect-Rejection 'Install Visual Studio 2022'
    Reset-Scenario; $script:vsExit = 1; Expect-Rejection 'Install Visual Studio 2022'
    Reset-Scenario; $script:targetExit = 1; Expect-Rejection 'query the Swift target'
    Reset-Scenario; $script:triple = 'aarch64-unknown-windows-msvc'; Expect-Rejection 'native x64 Swift toolchain'
    Write-Output 'Passed 10 toolchain scenarios (mocked boundaries; no native build).'
} finally {
    $env:PATH = $originalPath
    ${env:ProgramFiles(x86)} = $originalProgramFiles
}
