# Density7z framed stream format, version 1

The 7-Zip method ID already selects Chameleon, Cheetah, or Lion. The stream also
contains an algorithm byte so a mismatched or corrupted method is rejected.
All integer fields are unsigned little-endian.

## Stream header: 8 bytes

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 4 | ASCII magic `D7D1` |
| 4 | 1 | format version (`1`) |
| 5 | 1 | algorithm: `1` Chameleon, `2` Cheetah, `3` Lion |
| 6 | 1 | uncompressed chunk size as `log2`; encoder writes `22` (4 MiB) |
| 7 | 1 | flags/reserved; must be zero |

A decoder accepts chunk logs 16 through 26, which bounds a single chunk to
64 KiB through 64 MiB.

## Frame

Each frame starts with:

| Size | Meaning |
|---:|---|
| 4 | uncompressed size |
| 4 | packed size and flags |

Bit 31 of the packed-size field is the **stored/raw** flag. Bits 0 through 30
are the payload size. A raw frame must have equal packed and uncompressed
sizes. A compressed frame is decoded into exactly the declared uncompressed
size.

The frame header is followed by `packed_size` payload bytes. The encoder uses a
4 MiB working chunk and stores the chunk raw whenever Density does not make it
smaller.

## Terminator

A frame whose two 32-bit words are both zero ends the stream. Any other frame
with a zero size is invalid.

## Registered method IDs

These are private project assignments, not IDs allocated by a registry. They
follow 7-Zip's random-ID convention: prefix `3F`, randomly generated five-byte
developer ID `3C EF AE 04 60`, and a two-byte method number:

| Name | ID |
|---|---:|
| `DensityChameleon` | `0x3F3CEFAE04600001` |
| `DensityCheetah` | `0x3F3CEFAE04600002` |
| `DensityLion` | `0x3F3CEFAE04600003` |

Do not change these after distributing archives. Changing the framing requires
a new format version, and an incompatible algorithm change should receive a
new method ID.
