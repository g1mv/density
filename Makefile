#
# Centaurean Density
#
# Copyright (c) 2013, Guillaume Voirin
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
# 01/02/18 01:32
#

UPDATE_SUBMODULES := $(shell git submodule update --init --recursive)

TARGET = libdensity
CFLAGS = -Ofast -fomit-frame-pointer -flto -std=c99 -march=native -mtune=native -Wall -fPIC
LFLAGS = -flto

BUILD_DIRECTORY = ./build
DENSITY_BUILD_DIRECTORY = $(BUILD_DIRECTORY)/density
SRC_DIRECTORY = ./src

ifeq ($(OS),Windows_NT)
    bold =
    normal =
    ARROW = ^-^>
    EXTENSION = .dll
		BENCHMARK_EXTENSION = .exe
else
    bold = `tput bold`
    normal = `tput sgr0`
    ARROW = \-\>
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Darwin)
			EXTENSION = .dylib
    else
			EXTENSION = .so
		endif
		BENCHMARK_EXTENSION =
endif
STATIC_EXTENSION = .a

DENSITY_SRC = $(shell find $(SRC_DIRECTORY) -type f -name '*.c')
DENSITY_OBJ = $(patsubst $(SRC_DIRECTORY)%.c, $(DENSITY_BUILD_DIRECTORY)%.o, $(DENSITY_SRC))

.PHONY: pre-compile post-compile pre-link post-link library benchmark

all: benchmark

$(DENSITY_BUILD_DIRECTORY)%.o: $(SRC_DIRECTORY)/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

pre-compile:
	@echo ${bold}Compiling Density${normal} ...

compile: pre-compile $(DENSITY_OBJ)

post-compile: compile
	@echo Done.
	@echo

pre-link : post-compile
	@echo ${bold}Linking Density as a library${normal} ...

link: pre-link $(DENSITY_OBJ)
	$(AR) cr $(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION) $(DENSITY_OBJ)
	$(CC) $(LFLAGS) -shared -o $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION) $(DENSITY_OBJ)

post-link: link
	@echo Done.
	@echo
	@echo Static library file is here : ${bold}$(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION)${normal}
	@echo Dynamic library file is here : ${bold}$(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION)${normal}
	@echo

$(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION): post-link

library: post-link

benchmark: library
	@$(MAKE) -C benchmark/
	@echo Please type ${bold}$(BUILD_DIRECTORY)/density-benchmark${normal} to launch the benchmark binary.
	@echo

clean:
	@$(MAKE) -C benchmark/ clean
	@echo ${bold}Cleaning Density build files${normal} ...
	@rm -f $(DENSITY_OBJ)
	@rm -f $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION)
	@rm -f $(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION)
	@echo Done.
	@echo
