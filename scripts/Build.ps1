[CmdletBinding()]
param(
    [string]$QtRoot,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Test
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Tools.ps1')
$repoRoot = Split-Path $PSScriptRoot -Parent
$qtKit = Resolve-RobotArmQt $QtRoot
& (Join-Path $PSScriptRoot 'Generate-VS.ps1') -QtRoot $qtKit
$visualStudio = Get-RobotArmVisualStudio
Enter-RobotArmBuildEnvironment $visualStudio
$msbuild = Join-Path $visualStudio 'MSBuild\Current\Bin\MSBuild.exe'
if ($Test) {
    & (Join-Path $PSScriptRoot 'Test-LibrarySolutions.ps1') -Configuration $Configuration
}
& $msbuild (Join-Path $repoRoot 'RobotArmProject.sln') /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed for $Configuration|x64." }

if ($Test) {
    & $msbuild (Join-Path $repoRoot 'RobotArmUI\RobotArmUI.sln') /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw "Standalone UI solution failed for $Configuration|x64." }
    & (Join-Path $repoRoot "build\tests\x64\$Configuration\ArchitectureSmoke.exe")
    if ($LASTEXITCODE -ne 0) { throw 'Native DLL architecture checks failed.' }
    $ui = Join-Path $repoRoot "build\bin\x64\$Configuration\RobotArmUI.exe"
    $process = Start-Process -FilePath $ui -ArgumentList '--smoke-test', '-platform', 'windows' -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(15000)) {
        $process.Kill()
        throw 'Qt startup check timed out.'
    }
    if ($process.ExitCode -ne 0) { throw "Qt startup check failed: $($process.ExitCode)." }
    Write-Host "Verified: $Configuration|x64 DLL calls and Qt startup."
}
