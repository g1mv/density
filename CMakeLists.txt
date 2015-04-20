#
# Centaurean Density
#
# Copyright (c) 2015, Guillaume Voirin
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     1. Redistributions of source code must retain the above copyright notice, this
#        list of conditions and the following disclaimer.
#
#     2. Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation
#        and/or other materials provided with the distribution.
#
#     3. Neither the name of the copyright holder nor the names of its
#        contributors may be used to endorse or promote products derived from
#        this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# 11/04/15 11:37
#

cmake_minimum_required(VERSION 2.8)

find_program(CLANG_COMPILER clang)
if(CLANG_COMPILER)
    set(CMAKE_C_COMPILER ${CLANG_COMPILER})
endif(CLANG_COMPILER)

project(density)

execute_process (COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

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