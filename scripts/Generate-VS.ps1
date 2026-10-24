[CmdletBinding()]
param([string]$QtRoot)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Tools.ps1')
$repoRoot = Split-Path $PSScriptRoot -Parent
$qtKit = Resolve-RobotArmQt $QtRoot
$visualStudio = Get-RobotArmVisualStudio
Enter-RobotArmBuildEnvironment $visualStudio
$uiDirectory = Join-Path $repoRoot 'RobotArmUI'

Push-Location $uiDirectory
try {
    & (Join-Path $qtKit 'bin\qmake.exe') -tp vc RobotArmUI.pro -o RobotArmUI.vcxproj
    if ($LASTEXITCODE -ne 0) { throw 'Qt qmake VS project generation failed.' }
} finally { Pop-Location }

$projectPath = Join-Path $uiDirectory 'RobotArmUI.vcxproj'
[xml]$project = Get-Content -LiteralPath $projectPath -Raw
$ns = 'http://schemas.microsoft.com/developer/msbuild/2003'
$manager = [System.Xml.XmlNamespaceManager]::new($project.NameTable)
$manager.AddNamespace('msb', $ns)
$project.SelectSingleNode('//msb:ProjectGuid', $manager).InnerText = '{418F5929-3365-4FF9-AAB2-740D8120369F}'
# Use the newest installed Windows 10/11 SDK on each developer machine.
$project.SelectSingleNode('//msb:WindowsTargetPlatformVersion', $manager).InnerText = '10.0'
$minimumSdk = $project.SelectSingleNode('//msb:WindowsTargetPlatformMinVersion', $manager)
if ($minimumSdk) { [void]$minimumSdk.ParentNode.RemoveChild($minimumSdk) }
$targets = $project.SelectSingleNode('/msb:Project/msb:Import[contains(@Project,"Microsoft.Cpp.targets")]', $manager)

$settings = $project.CreateElement('PropertyGroup', $ns)
$qtProperty = $project.CreateElement('RobotArmQtRoot', $ns)
$qtProperty.InnerText = $qtKit
[void]$settings.AppendChild($qtProperty)
[void]$project.DocumentElement.InsertBefore($settings, $targets)
$import = $project.CreateElement('Import', $ns)
$import.SetAttribute('Project', 'RobotArmUI.props')
[void]$project.DocumentElement.InsertBefore($import, $targets)

$references = $project.CreateElement('ItemGroup', $ns)
foreach ($name in @('Common', 'Kinematics', 'Trajectory')) {
    [xml]$native = Get-Content -LiteralPath (Join-Path $repoRoot "$name\$name.vcxproj") -Raw
    $reference = $project.CreateElement('ProjectReference', $ns)
    $reference.SetAttribute('Include', "..\$name\$name.vcxproj")
    $guid = $project.CreateElement('Project', $ns)
    $guid.InnerText = $native.SelectSingleNode('//*[local-name()="ProjectGuid"]').InnerText
    [void]$reference.AppendChild($guid)
    $link = $project.CreateElement('LinkLibraryDependencies', $ns)
    $link.InnerText = 'false'
    [void]$reference.AppendChild($link)
    [void]$references.AppendChild($reference)
}
[void]$project.DocumentElement.InsertBefore($references, $targets)
$project.Save($projectPath)

function Write-RobotArmSolution([string]$Destination, [bool]$UiOnly) {
    $entries = @(
        @{ Name = 'RobotArmUI'; Path = 'RobotArmUI\RobotArmUI.vcxproj' },
        @{ Name = 'Common'; Path = 'Common\Common.vcxproj' },
        @{ Name = 'Kinematics'; Path = 'Kinematics\Kinematics.vcxproj' },
        @{ Name = 'Trajectory'; Path = 'Trajectory\Trajectory.vcxproj' }
    )
    if (-not $UiOnly) { $entries += @{ Name = 'ArchitectureSmoke'; Path = 'tests\ArchitectureSmoke.vcxproj' } }
    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('Microsoft Visual Studio Solution File, Format Version 12.00')
    $lines.Add('# Visual Studio Version 17')
    $lines.Add('VisualStudioVersion = 17.0.31903.59')
    $lines.Add('MinimumVisualStudioVersion = 10.0.40219.1')
    foreach ($entry in $entries) {
        [xml]$entryXml = Get-Content -LiteralPath (Join-Path $repoRoot $entry.Path) -Raw
        $entry.Guid = $entryXml.SelectSingleNode('//*[local-name()="ProjectGuid"]').InnerText.ToUpperInvariant()
        $relative = $entry.Path
        if ($UiOnly) {
            if ($entry.Name -eq 'RobotArmUI') { $relative = 'RobotArmUI.vcxproj' }
            else { $relative = '..\' + $relative }
        }
        $lines.Add(('Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{0}", "{1}", "{2}"' -f $entry.Name, $relative, $entry.Guid))
        $lines.Add('EndProject')
    }
    $lines.Add('Global')
    $lines.Add("`tGlobalSection(SolutionConfigurationPlatforms) = preSolution")
    foreach ($config in @('Debug', 'Release')) { $lines.Add("`t`t$config|x64 = $config|x64") }
    $lines.Add("`tEndGlobalSection")
    $lines.Add("`tGlobalSection(ProjectConfigurationPlatforms) = postSolution")
    foreach ($entry in $entries) {
        foreach ($config in @('Debug', 'Release')) {
            $lines.Add("`t`t$($entry.Guid).$config|x64.ActiveCfg = $config|x64")
            $lines.Add("`t`t$($entry.Guid).$config|x64.Build.0 = $config|x64")
        }
    }
    $lines.Add("`tEndGlobalSection")
    $lines.Add('EndGlobal')
    [System.IO.File]::WriteAllText($Destination, ($lines -join "`r`n") + "`r`n", [System.Text.UTF8Encoding]::new($false))
}

Write-RobotArmSolution (Join-Path $repoRoot 'RobotArmProject.sln') $false
Write-RobotArmSolution (Join-Path $uiDirectory 'RobotArmUI.sln') $true
Write-Host "Generated RobotArmProject.sln and RobotArmUI\RobotArmUI.sln from the Qt pro project."
