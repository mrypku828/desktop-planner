param(
    [string]$QtRoot = 'D:\qt\6.11.1\mingw_64',
    [string]$CompilerBin = 'D:\qt\Tools\mingw1310_64\bin',
    [string]$Ninja = 'D:\qt\Tools\Ninja\ninja.exe',
    [string]$CMake = (Get-Command cmake -ErrorAction Stop).Source,
    [string]$BuildDirectory = (Join-Path $env:LOCALAPPDATA 'DesktopPlannerBuild'),
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$RunTests
)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$env:PATH = "$CompilerBin;$QtRoot\bin;" + $env:PATH
& $CMake -S $projectDirectory -B $BuildDirectory -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" "-DCMAKE_PREFIX_PATH=$QtRoot" "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe" "-DCMAKE_MAKE_PROGRAM=$Ninja" '-DBUILD_TESTING=ON'
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $CMake --build $BuildDirectory
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
if ($RunTests) {
    $ctest = Join-Path (Split-Path $CMake -Parent) 'ctest.exe'
    & $ctest --test-dir $BuildDirectory --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed. Inspect Testing/Temporary/LastTest.log and test-results.txt in the build directory.' }
}
Write-Output "Built application: $BuildDirectory\DesktopPlanner.exe"
