param(
    [string]$BuildDirectory = (Join-Path $env:LOCALAPPDATA 'DesktopPlannerBuild'),
    [string]$QtRoot = 'D:\qt\6.11.1\mingw_64',
    [string]$CompilerBin = 'D:\qt\Tools\mingw1310_64\bin',
    [string]$Destination
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
if (-not $Destination) { $Destination = Join-Path $projectDirectory 'dist\DesktopPlanner-v1.1.1' }
# Refuse to overwrite a previous release, so custom/user files cannot be silently replaced.
if (Test-Path -LiteralPath $Destination) { throw 'Destination already exists. Choose a new -Destination.' }
$executable = Join-Path $BuildDirectory 'DesktopPlanner.exe'
if (-not (Test-Path -LiteralPath $executable)) { throw 'Build DesktopPlanner first.' }
New-Item -ItemType Directory -Path $Destination | Out-Null
Copy-Item -LiteralPath $executable -Destination $Destination
$env:PATH = "$CompilerBin;$QtRoot\bin;" + $env:PATH
& "$QtRoot\bin\windeployqt.exe" --release --translations zh_CN --no-opengl-sw --compiler-runtime --dir $Destination "$Destination\DesktopPlanner.exe"
if ($LASTEXITCODE -ne 0) { throw 'windeployqt failed.' }
if (-not (Test-Path -LiteralPath "$Destination\sqldrivers\qsqlite.dll")) {
    New-Item -ItemType Directory -Path "$Destination\sqldrivers" -Force | Out-Null
    Copy-Item -LiteralPath "$QtRoot\plugins\sqldrivers\qsqlite.dll" -Destination "$Destination\sqldrivers"
}
Copy-Item -LiteralPath "$projectDirectory\README.md" -Destination "$Destination\README.md"
New-Item -ItemType Directory -Path "$Destination\translations" -Force | Out-Null
Copy-Item -LiteralPath "$QtRoot\translations\qtbase_zh_CN.qm" -Destination "$Destination\translations"
if (Test-Path "$projectDirectory\docs\TEST_RESULTS.md") { Copy-Item -LiteralPath "$projectDirectory\docs\TEST_RESULTS.md" -Destination $Destination }
if (Test-Path "$projectDirectory\docs") { Copy-Item -LiteralPath "$projectDirectory\docs" -Destination "$Destination\docs" -Recurse }
$qtInstallationRoot = Split-Path (Split-Path $QtRoot -Parent) -Parent
if (Test-Path "$qtInstallationRoot\Licenses") {
    New-Item -ItemType Directory -Path "$Destination\licenses" -Force | Out-Null
    Copy-Item -LiteralPath "$qtInstallationRoot\Licenses" -Destination "$Destination\licenses\Qt" -Recurse
}
Write-Output "Release folder: $Destination"
