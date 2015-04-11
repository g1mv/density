cmake_minimum_required(VERSION 2.8)

project(benchmark)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -fomit-frame-pointer -flto -std=c99")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -flto")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable (
    benchmark
    src/benchmark.c
)

target_link_libraries (benchmark LINK_PUBLIC spookyhash)
target_link_libraries (benchmark LINK_PUBLIC density)