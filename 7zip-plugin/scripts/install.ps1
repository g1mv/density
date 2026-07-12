[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Dll,

    [string]$SevenZipDirectory = "$env:ProgramFiles\7-Zip"
)

$ErrorActionPreference = 'Stop'
$Dll = (Resolve-Path $Dll).Path
$SevenZipDirectory = (Resolve-Path $SevenZipDirectory).Path
$CodecDirectory = Join-Path $SevenZipDirectory 'Codecs'

New-Item -ItemType Directory -Path $CodecDirectory -Force | Out-Null
Copy-Item $Dll (Join-Path $CodecDirectory 'DensityCodec.dll') -Force

$SevenZip = Join-Path $SevenZipDirectory '7z.exe'
if (Test-Path $SevenZip) {
    & $SevenZip i | Select-String -Pattern 'Density'
}

Write-Host "Installed: $(Join-Path $CodecDirectory 'DensityCodec.dll')"
