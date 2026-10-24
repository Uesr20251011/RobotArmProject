[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Tools.ps1')
$repoRoot = Split-Path $PSScriptRoot -Parent
$visualStudio = Get-RobotArmVisualStudio
Enter-RobotArmBuildEnvironment $visualStudio
$msbuild = Join-Path $visualStudio 'MSBuild\Current\Bin\MSBuild.exe'

# Each solution must build its own dependency closure without Qt or the root solution.
foreach ($name in @('Common', 'Kinematics', 'Trajectory')) {
    $solution = Join-Path $repoRoot "$name\$name.sln"
    & $msbuild $solution /t:Rebuild /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw "Standalone solution failed: $name ($Configuration|x64)." }
    Write-Host "Verified standalone solution: $name ($Configuration|x64)."
}
