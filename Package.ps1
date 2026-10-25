param(
    [string]$Build = "$PSScriptRoot/build",
    [string]$Juce = "$PSScriptRoot/build/_deps/juce-src",
    [string]$Destination = "$PSScriptRoot/dist"
)
$ErrorActionPreference = 'Stop'
$binary = Join-Path $Build 'KernDrum_artefacts/Release'
if (!(Test-Path "$binary/VST3/KernDrum.vst3")) { throw 'Build the Release VST3 first.' }
if (!(Test-Path "$Juce/LICENSE.md")) { throw 'Supply the pinned JUCE source directory.' }
if (Test-Path $Destination) { throw 'Use a new destination to avoid overwriting an existing release.' }
$package = New-Item -ItemType Directory "$Destination/KernDrum-Windows-x64"
Copy-Item "$binary/VST3/KernDrum.vst3" $package -Recurse
Copy-Item "$binary/Standalone/KernDrum.exe" $package
Copy-Item "$PSScriptRoot/README.md","$PSScriptRoot/LICENSE","$PSScriptRoot/THIRD_PARTY.md","$PSScriptRoot/TEST_REPORT.md" $package
$licenses = New-Item -ItemType Directory "$package/Licenses"
Copy-Item "$Juce/LICENSE.md" "$licenses/JUCE-LICENSE.md"
$moduleRoot = (Resolve-Path "$Juce/modules").Path
Get-ChildItem $moduleRoot -File -Recurse | Where-Object { $_.Name -match '(?i)license|licence|copying|notice|readme|authors' } | ForEach-Object {
    $relative = [IO.Path]::GetRelativePath($moduleRoot, $_.FullName)
    $target = Join-Path $licenses $relative
    New-Item -ItemType Directory -Force (Split-Path $target) | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $target
}
Compress-Archive "$package/*" "$Destination/KernDrum-Windows-x64.zip"
$source = New-Item -ItemType Directory "$Destination/Source/KernDrum"
Get-ChildItem $PSScriptRoot -Force | Where-Object { $_.Name -notin @('.git','build','dist','.vs') -and !$_.Name.StartsWith('build-') } | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $source -Recurse }
$vendor = New-Item -ItemType Directory "$Destination/Source/JUCE"
Get-ChildItem $Juce -Force | Where-Object Name -ne '.git' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $vendor -Recurse }
Compress-Archive "$Destination/Source/*" "$Destination/KernDrum-Complete-Source.zip"
Get-FileHash "$Destination/*.zip" -Algorithm SHA256 | ForEach-Object { "$($_.Hash.ToLower())  $([IO.Path]::GetFileName($_.Path))" } | Set-Content "$Destination/SHA256SUMS.txt"
Write-Output "Release packages: $Destination"
