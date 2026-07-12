cmake_minimum_required(VERSION 3.24)

project(Density7ZipCodec LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "The external 7-Zip codec DLL target is Windows-only.")
endif()
if(NOT MSVC)
  message(FATAL_ERROR "This starter currently supports the MSVC toolchain only.")
endif()

set(SEVENZIP_SOURCE_DIR "" CACHE PATH "Root of the official ip7z/7zip source tree")
set(DENSITY_RUST_TARGET "" CACHE STRING "Rust MSVC target triple")

if(NOT SEVENZIP_SOURCE_DIR)
  message(FATAL_ERROR "Set -DSEVENZIP_SOURCE_DIR=C:/path/to/7zip")
endif()

set(SEVENZIP_CPP "${SEVENZIP_SOURCE_DIR}/CPP")
foreach(required_file
    "${SEVENZIP_CPP}/7zip/Compress/CodecExports.cpp"
    "${SEVENZIP_CPP}/7zip/Compress/DllExportsCompress.cpp"
    "${SEVENZIP_CPP}/7zip/Common/RegisterCodec.h")
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "Missing 7-Zip source file: ${required_file}")
  endif()
endforeach()

find_program(CARGO_EXECUTABLE cargo REQUIRED)

if(NOT DENSITY_RUST_TARGET)
  if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
    set(DENSITY_RUST_TARGET "aarch64-pc-windows-msvc")
  elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    set(DENSITY_RUST_TARGET "i686-pc-windows-msvc")
  else()
    set(DENSITY_RUST_TARGET "x86_64-pc-windows-msvc")
  endif()
endif()

set(RUST_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml")
set(RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo-target")
set(RUST_LIBRARY
    "${RUST_TARGET_DIR}/${DENSITY_RUST_TARGET}/release/density_7zip_bridge.lib")

file(GLOB_RECURSE BRIDGE_RUST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/rust/src/*.rs")
file(GLOB_RECURSE DENSITY_RUST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.rs")

add_custom_command(
  OUTPUT "${RUST_LIBRARY}"
  COMMAND "${CMAKE_COMMAND}" -E env
          "CARGO_TARGET_DIR=${RUST_TARGET_DIR}"
          "${CARGO_EXECUTABLE}" build
          --manifest-path "${RUST_MANIFEST}"
          --target "${DENSITY_RUST_TARGET}"
          --release
  DEPENDS
    "${RUST_MANIFEST}"
    ${BRIDGE_RUST_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/../Cargo.toml"
    ${DENSITY_RUST_SOURCES}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Building the Rust Density bridge"
  VERBATIM)

add_custom_target(density_rust_bridge ALL DEPENDS "${RUST_LIBRARY}")

add_library(DensityCodec SHARED
  cpp/DensityCoder.cpp
  cpp/DensityCoder.h
  cpp/DensityBridge.h
  cpp/DensityRegister.cpp
  cpp/DensityCodec.def
  "${SEVENZIP_CPP}/7zip/Compress/CodecExports.cpp"
  "${SEVENZIP_CPP}/7zip/Compress/DllExportsCompress.cpp")

add_dependencies(DensityCodec density_rust_bridge)

target_compile_features(DensityCodec PRIVATE cxx_std_17)
target_compile_definitions(DensityCodec PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX)
target_compile_options(DensityCodec PRIVATE /EHsc /utf-8)
target_include_directories(DensityCodec PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
  "${SEVENZIP_CPP}"
  "${SEVENZIP_SOURCE_DIR}/C")
target_link_libraries(DensityCodec PRIVATE
  "${RUST_LIBRARY}"
  oleaut32
  ole32
  advapi32
  bcrypt
  dbghelp
  kernel32
  userenv
  ws2_32
  ntdll)

set_target_properties(DensityCodec PROPERTIES
  PREFIX ""
  OUTPUT_NAME "DensityCodec"
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
