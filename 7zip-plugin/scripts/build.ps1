[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SevenZipSource,

    [ValidateSet('x64', 'Win32', 'ARM64')]
    [string]$Architecture = 'x64',

    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',

    [string]$BuildDirectory = ''
)

$ErrorActionPreference = 'Stop'
$PluginRoot = Split-Path -Parent $PSScriptRoot

if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $PluginRoot "build-$Architecture"
}

$RustTarget = switch ($Architecture) {
    'Win32' { 'i686-pc-windows-msvc' }
    'ARM64' { 'aarch64-pc-windows-msvc' }
    default { 'x86_64-pc-windows-msvc' }
}

foreach ($Command in @('cmake', 'cargo', 'rustup')) {
    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) {
        throw "Required command '$Command' was not found in PATH."
    }
}

$SevenZipSource = (Resolve-Path $SevenZipSource).Path
rustup target add $RustTarget
if ($LASTEXITCODE -ne 0) {
    throw "rustup could not install target $RustTarget."
}

cmake -S $PluginRoot -B $BuildDirectory `
    -A $Architecture `
    -DSEVENZIP_SOURCE_DIR="$SevenZipSource" `
    -DDENSITY_RUST_TARGET="$RustTarget"
if ($LASTEXITCODE -ne 0) {
    throw 'CMake configuration failed.'
}

cmake --build $BuildDirectory --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw 'CMake build failed.'
}

$Dll = Join-Path $BuildDirectory "bin\$Configuration\DensityCodec.dll"
if (-not (Test-Path $Dll)) {
    $Dll = Join-Path $BuildDirectory "bin\DensityCodec.dll"
}
if (-not (Test-Path $Dll)) {
    throw "Build completed but DensityCodec.dll was not found."
}

Write-Host "Built: $Dll"
