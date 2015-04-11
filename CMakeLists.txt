cmake_minimum_required(VERSION 2.8)

project(density)

execute_process (COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -fomit-frame-pointer -flto -std=c99")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -flto")

add_subdirectory (src/spookyhash)
add_subdirectory (src/benchmark)

add_library (
    density
    src/block_decode.c
    src/block_encode.c
    src/block_footer.c
    src/block_header.c
    src/block_mode_marker.c
    src/buffer.c
    src/globals.c
    src/kernel_chameleon_decode.c
    src/kernel_chameleon_dictionary.c
    src/kernel_chameleon_encode.c
    src/kernel_cheetah_decode.c
    src/kernel_cheetah_dictionary.c
    src/kernel_cheetah_encode.c
    src/kernel_lion_decode.c
    src/kernel_lion_dictionary.c
    src/kernel_lion_encode.c
    src/kernel_lion_form_model.c
    src/main_decode.c
    src/main_encode.c
    src/main_footer.c
    src/main_header.c
    src/memory_location.c
    src/memory_teleport.c
    src/stream.c
)