[CmdletBinding()]
param(
    [string]$Version = '3.0.0',
    [string]$BuildDirectory = 'D:\AI-Projects\02_Builds\MetricBar',
    [string]$ArtifactDirectory = 'D:\AI-Projects\03_Artifacts\MetricBar'
)

$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$packageName = "MetricBar-$Version-win10-x64"
$stagingParent = Join-Path $BuildDirectory 'package'
$stagingDirectory = Join-Path $stagingParent $packageName
$archivePath = Join-Path $ArtifactDirectory ($packageName + '.tar.gz')
$archiveHashPath = $archivePath + '.sha256'

foreach ($path in @($stagingDirectory, $archivePath, $archiveHashPath)) {
    if (Test-Path -LiteralPath $path) {
        throw "Refusing to overwrite existing package output: $path"
    }
}

New-Item -ItemType Directory -Path $stagingDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $ArtifactDirectory -Force | Out-Null
$sourceDirectory = Join-Path $stagingDirectory 'source'
New-Item -ItemType Directory -Path $sourceDirectory -Force | Out-Null

$runtimeFiles = @('VpsTraySpeed.dll', 'config.ini', 'install.bat', 'uninstall.bat')
foreach ($name in $runtimeFiles) {
    Copy-Item -LiteralPath (Join-Path $BuildDirectory $name) -Destination $stagingDirectory
}
foreach ($name in @('README.md', 'README.zh-CN.md', 'LICENSE', 'PUBLISHING.md')) {
    Copy-Item -LiteralPath (Join-Path $projectDirectory $name) -Destination $stagingDirectory
    Copy-Item -LiteralPath (Join-Path $projectDirectory $name) -Destination $sourceDirectory
}
Copy-Item -LiteralPath (Join-Path $projectDirectory 'docs') -Destination $stagingDirectory -Recurse
Copy-Item -LiteralPath (Join-Path $projectDirectory 'docs') -Destination $sourceDirectory -Recurse
Copy-Item -LiteralPath (Join-Path $projectDirectory 'src') -Destination $sourceDirectory -Recurse
Copy-Item -LiteralPath (Join-Path $projectDirectory 'tests') -Destination $sourceDirectory -Recurse
foreach ($name in @('.gitattributes', '.gitignore', 'Makefile', 'build.ps1', 'package.ps1', 'config.ini', 'install.bat', 'uninstall.bat')) {
    Copy-Item -LiteralPath (Join-Path $projectDirectory $name) -Destination $sourceDirectory
}

$hashLines = foreach ($name in $runtimeFiles) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $stagingDirectory $name) -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $name"
}
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText((Join-Path $stagingDirectory 'SHA256SUMS.txt'),
    (($hashLines -join "`n") + "`n"), $utf8NoBom)

& tar.exe -czf $archivePath -C $stagingParent $packageName
if ($LASTEXITCODE -ne 0) { throw "tar failed with exit code $LASTEXITCODE." }
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText($archiveHashPath, "$archiveHash  $($packageName).tar.gz`n", $utf8NoBom)
Write-Host "Created $archivePath"
Write-Host "SHA256 $archiveHash"
