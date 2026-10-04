[CmdletBinding()]
param(
    [string]$Distro = 'Ubuntu-22.04',
    [string]$OutputDirectory = 'D:\AI-Projects\02_Builds\MetricBar'
)

$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path

function Convert-ToWslPath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if ($resolved -notmatch '^([A-Za-z]):\\(.*)$') {
        throw "Only drive-letter Windows paths are supported: $resolved"
    }
    $drive = $Matches[1].ToLowerInvariant()
    $tail = $Matches[2] -replace '\\', '/'
    return "/mnt/$drive/$tail"
}

$wslProject = Convert-ToWslPath $projectDirectory
$wslOutput = Convert-ToWslPath $OutputDirectory
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

& wsl.exe -d $Distro -- make -C $wslProject "BUILD_DIR=$wslOutput" clean all tests
if ($LASTEXITCODE -ne 0) { throw "Native build failed with exit code $LASTEXITCODE." }

Copy-Item -LiteralPath (Join-Path $projectDirectory 'config.ini') -Destination $OutputDirectory -Force
Copy-Item -LiteralPath (Join-Path $projectDirectory 'install.bat') -Destination $OutputDirectory -Force
Copy-Item -LiteralPath (Join-Path $projectDirectory 'uninstall.bat') -Destination $OutputDirectory -Force

$testExe = Join-Path $OutputDirectory 'NetParsingTests.exe'
& $testExe
if ($LASTEXITCODE -ne 0) { throw "Native tests failed with exit code $LASTEXITCODE." }

$contractTestExe = Join-Path $OutputDirectory 'ComContractTests.exe'
$dll = Join-Path $OutputDirectory 'VpsTraySpeed.dll'
& $contractTestExe $dll
if ($LASTEXITCODE -ne 0) { throw "COM contract tests failed with exit code $LASTEXITCODE." }

$integrationTestExe = Join-Path $OutputDirectory 'IntegrationHostTests.exe'
$previewBmp = Join-Path $OutputDirectory 'band-preview.bmp'
& $integrationTestExe $dll success $previewBmp
if ($LASTEXITCODE -ne 0) { throw "DeskBand integration test failed with exit code $LASTEXITCODE." }

$bytes = [IO.File]::ReadAllBytes($dll)
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
$machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
if ($machine -ne 0x8664) { throw ('Expected x64 PE machine 0x8664, found 0x{0:X4}.' -f $machine) }
Write-Host "Built and tested x64 DLL: $dll"
