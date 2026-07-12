# Licensing notes

The Rust Density dependency is licensed `MIT OR Apache-2.0` by its authors.
The bridge and shim in this starter are intended to be kept under the same
dual-license terms when added to that repository.

The build compiles `CodecExports.cpp` and `DllExportsCompress.cpp` from the
separately obtained official 7-Zip source tree into the DLL. 7-Zip has its own
license terms, including LGPL-covered code. Keep the corresponding 7-Zip
source/version and this buildable glue available with any binary distribution,
and review the official 7-Zip license for your distribution model. This note
is not legal advice.

No 7-Zip source file is copied into this starter archive.
