Set-StrictMode -Version Latest

function Get-RobotArmVisualStudio {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2022 with Desktop development with C++.' }
    $installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Visual Studio 2022 C++ tools were not found.' }
    return [string]$installation
}

function Enter-RobotArmBuildEnvironment([string]$VisualStudioRoot) {
    Import-Module (Join-Path $VisualStudioRoot 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $VisualStudioRoot -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}

function Resolve-RobotArmQt([string]$QtRoot) {
    if (-not $QtRoot) { $QtRoot = $env:QT_ROOT }
    if (-not $QtRoot) { throw 'Provide -QtRoot with the Qt 6.11.2 MSVC 2022 x64 kit directory, or set QT_ROOT.' }
    $resolved = (Resolve-Path -LiteralPath $QtRoot -ErrorAction Stop).Path
    $qmake = Join-Path $resolved 'bin\qmake.exe'
    if (-not (Test-Path -LiteralPath $qmake)) { throw "qmake.exe was not found under $resolved" }
    $spec = & $qmake -query QMAKE_XSPEC
    if ($LASTEXITCODE -ne 0 -or $spec -ne 'win32-msvc') { throw 'Qt must use the MSVC kit; MinGW is not supported.' }
    $version = & $qmake -query QT_VERSION
    if ($LASTEXITCODE -ne 0 -or $version -ne '6.11.2') { throw 'All collaborators must use Qt 6.11.2 MSVC 2022 x64.' }
    return $resolved
}
