# Density external codec plugin for 7-Zip

This directory is intended to live at `density/7zip-plugin`. It builds a
Windows external-codec DLL that registers three 7-Zip compression methods:

- `DensityChameleon`
- `DensityCheetah`
- `DensityLion`

The implementation consists of a small Rust static library around the existing
one-shot Density API and a C++ adapter implementing 7-Zip's `ICompressCoder`.
The adapter uses a framed, chunked stream, so it does not retain an entire solid
archive block in memory. The default chunk is 4 MiB.

## Scope

This lets 7-Zip use Density inside `.7z` archives. It is a codec plugin, not a
file-format plugin, so it does not add a standalone `.density` archive type or
raw-stream handler.

Archives using these methods need a compatible `DensityCodec.dll` installed to
extract them. The method IDs and framing are private to this project; see
[`FORMAT.md`](FORMAT.md).

## Prerequisites

- Windows 10 or later
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.24+
- Rust stable and `rustup`
- Official 7-Zip source matching the build you intend to support; the included
  CI pins tag `26.02` and builds x64, x86, and ARM64 DLLs

The adapter is set up for MSVC targets (`x86_64`, `i686`, and `aarch64`). The
DLL architecture must match `7z.exe`.

## Repository layout

The dependency path in `rust/Cargo.toml` assumes:

```text
density/
  Cargo.toml
  src/
  7zip-plugin/
    CMakeLists.txt
    cpp/
    rust/
```

## Build

Obtain the official 7-Zip 26.02 source tree separately, then run a Developer
PowerShell:

```powershell
cd C:\src\density\7zip-plugin
.\scripts\build.ps1 -SevenZipSource C:\src\7zip -Architecture x64
```

The expected result is:

```text
7zip-plugin\build-x64\bin\Release\DensityCodec.dll
```

Equivalent direct CMake commands:

```powershell
rustup target add x86_64-pc-windows-msvc
cmake -S . -B build-x64 -A x64 `
  -DSEVENZIP_SOURCE_DIR=C:\src\7zip `
  -DDENSITY_RUST_TARGET=x86_64-pc-windows-msvc
cmake --build build-x64 --config Release
```

Run the Rust bridge tests separately:

```powershell
cargo test --manifest-path rust\Cargo.toml
```

## Install

Run an elevated PowerShell because the default location is under Program
Files:

```powershell
.\scripts\install.ps1 `
  -Dll .\build-x64\bin\Release\DensityCodec.dll
```

This copies the DLL to `%ProgramFiles%\7-Zip\Codecs`. Verify discovery:

```powershell
& "$env:ProgramFiles\7-Zip\7z.exe" i | Select-String Density
```

## Create and verify archives

```powershell
7z a sample-cheetah.7z .\input.bin -m0=DensityCheetah
7z t sample-cheetah.7z
7z x sample-cheetah.7z -o.\out -y
fc.exe /b .\input.bin .\out\input.bin
```

Replace the method with `DensityChameleon` or `DensityLion` for the other
algorithms. Stock 7-Zip builds can load external codecs without showing them in
the Add-to-archive method drop-down, so the command line (or an equivalent
advanced-parameters field) is the reliable way to select these methods.

After installation, the included automated smoke test covers all three methods
with empty, repetitive, cross-chunk random, solid, and partial-solid inputs:

```powershell
.\scripts\smoke-test.ps1
```

Test at minimum:

- empty and tiny files
- incompressible random data
- inputs around 4 MiB chunk boundaries
- multi-gigabyte files
- solid and non-solid archives
- damaged and truncated archives
- matching x64, x86, and ARM64 DLL/7-Zip pairs as applicable

## Design constraints

Density currently exposes one-shot in-memory operations. The codec resets
Density state for every frame. Memory stays bounded, streaming through 7-Zip
works, and corruption can be rejected frame by frame; the tradeoff is that
compression cannot learn across 4 MiB frame boundaries.

The Rust bridge validates pointers and catches panics so Rust unwinding never
crosses the C ABI. The decoder allocates the exact declared output size for a
frame, checks Density's decoded byte count, respects 7-Zip's `outSize`, supports
partial decoding for single-file extraction from solid archives, and returns
`S_FALSE` for malformed data. It implements `ICompressSetFinishMode`, so 7-Zip
can request full-stream validation when required.

## Before publishing archives

1. Freeze the method IDs and `FORMAT.md`; the included IDs use a random
   five-byte developer ID as recommended by 7-Zip.
2. Add golden archives for all three methods to CI.
3. Build and test against every 7-Zip version/architecture you support.
4. Fuzz the framed decoder and Rust bridge.
5. Decide whether a per-frame checksum is needed before freezing version 1.
   The `.7z` container already supplies archive/file integrity checks.
6. Publish the DLL, source, build instructions, and applicable license notices
   together.
