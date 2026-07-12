[CmdletBinding()]
param(
    [string]$SevenZip = "$env:ProgramFiles\7-Zip\7z.exe",
    [string]$WorkingDirectory = ''
)

$ErrorActionPreference = 'Stop'
$SevenZip = (Resolve-Path $SevenZip).Path

if (-not $WorkingDirectory) {
    $WorkingDirectory = Join-Path $env:TEMP 'density-7zip-smoke'
}
if (Test-Path $WorkingDirectory) {
    Remove-Item $WorkingDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path $WorkingDirectory | Out-Null

$Inputs = Join-Path $WorkingDirectory 'inputs'
New-Item -ItemType Directory -Path $Inputs | Out-Null

[IO.File]::WriteAllBytes((Join-Path $Inputs 'empty.bin'), [byte[]]@())
$RepetitiveText = [string]::new([char]'D', [int]((4MB) + 65537))
[IO.File]::WriteAllText(
    (Join-Path $Inputs 'repetitive.txt'),
    $RepetitiveText,
    [Text.Encoding]::ASCII)

$RandomBytes = [byte[]]::new([int]((4MB) + 123))
$Rng = [Security.Cryptography.RandomNumberGenerator]::Create()
try {
    $Rng.GetBytes($RandomBytes)
} finally {
    $Rng.Dispose()
}
[IO.File]::WriteAllBytes((Join-Path $Inputs 'random.bin'), $RandomBytes)

$Methods = @('DensityChameleon', 'DensityCheetah', 'DensityLion')
$Files = @(Get-ChildItem $Inputs -File | Sort-Object Name)

foreach ($Method in $Methods) {
    foreach ($File in $Files) {
        $Case = "$Method-$($File.BaseName)"
        $Archive = Join-Path $WorkingDirectory "$Case.7z"
        $Output = Join-Path $WorkingDirectory "out-$Case"
        New-Item -ItemType Directory -Path $Output | Out-Null

        Push-Location $Inputs
        try {
            & $SevenZip a -t7z $Archive $File.Name "-m0=$Method" -y | Out-Host
            if ($LASTEXITCODE -ne 0) { throw "7z add failed for $Case" }
        } finally {
            Pop-Location
        }

        & $SevenZip t $Archive | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "7z test failed for $Case" }

        & $SevenZip x $Archive "-o$Output" -y | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "7z extract failed for $Case" }

        $Extracted = Join-Path $Output $File.Name
        $OriginalHash = (Get-FileHash $File.FullName -Algorithm SHA256).Hash
        $ExtractedHash = (Get-FileHash $Extracted -Algorithm SHA256).Hash
        if ($OriginalHash -ne $ExtractedHash) {
            throw "SHA-256 mismatch for $Case"
        }
    }

    $SolidArchive = Join-Path $WorkingDirectory "${Method}-solid.7z"
    $SolidOutput = Join-Path $WorkingDirectory "out-${Method}-solid"
    New-Item -ItemType Directory -Path $SolidOutput | Out-Null
    Push-Location $Inputs
    try {
        & $SevenZip a -t7z $SolidArchive '*' "-m0=$Method" -ms=on -mqs=off -y | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "7z solid add failed for $Method" }
    } finally {
        Pop-Location
    }
    & $SevenZip t $SolidArchive | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "7z solid test failed for $Method" }
    & $SevenZip x $SolidArchive "-o$SolidOutput" -y | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "7z solid extract failed for $Method" }
    foreach ($File in $Files) {
        $Extracted = Join-Path $SolidOutput $File.Name
        if ((Get-FileHash $File.FullName -Algorithm SHA256).Hash -ne
            (Get-FileHash $Extracted -Algorithm SHA256).Hash) {
            throw "SHA-256 mismatch for $Method solid archive: $($File.Name)"
        }
    }

    # Extract one member from the solid stream. This exercises 7-Zip's
    # partial-decoding path, including an output boundary inside a codec frame.
    $PartialFile = $Files | Where-Object { $_.Length -gt 0 } | Select-Object -First 1
    $PartialOutput = Join-Path $WorkingDirectory "out-${Method}-solid-partial"
    New-Item -ItemType Directory -Path $PartialOutput | Out-Null
    & $SevenZip x $SolidArchive $PartialFile.Name "-o$PartialOutput" -y | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "7z partial solid extract failed for $Method"
    }
    $PartialExtracted = Join-Path $PartialOutput $PartialFile.Name
    if ((Get-FileHash $PartialFile.FullName -Algorithm SHA256).Hash -ne
        (Get-FileHash $PartialExtracted -Algorithm SHA256).Hash) {
        throw "SHA-256 mismatch for $Method partial solid extraction"
    }
}

Write-Host "All Density codec smoke tests passed in $WorkingDirectory"
