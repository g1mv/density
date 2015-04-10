#
# Centaurean Density
#
# Copyright (c) 2015, Guillaume Voirin
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Centaurean nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# 10/04/15 16:08
#

UPDATE_SUBMODULES := $(shell git submodule update --init --recursive)

DENSITY = density
BENCHMARK = benchmark
CFLAGS = -Ofast -fomit-frame-pointer -fPIC -std=c99

SRC_DIRECTORY = ./src/
SPOOKYHASH_DIRECTORY = ./src/spookyhash/
SPOOKYHASH_SRC_DIRECTORY = $(SPOOKYHASH_DIRECTORY)src/
BENCHMARK_DIRECTORY = ./src/benchmark/
BENCHMARK_SRC_DIRECTORY = $(BENCHMARK_DIRECTORY)src/

# Search for Clang
CLANG_COMPILER := $(@shell clang -v)
FLTO = -flto
ifeq ($(CLANG_COMPILER),"")
	COMPILER = $(CC)
else
	COMPILER = clang
	ifeq ($(OS),Windows_NT)
		FLTO = 
	endif
endif

# OS specifics
ifeq ($(OS),Windows_NT)
    bold =
    normal =
    STAT_EXT = .lib
    DYN_EXT = .dll
    ARROW = ^-^>
    EXTENSION = .exe
else
    bold = `tput bold`
    normal = `tput sgr0`
    UNAME_S := $(@shell uname -s)
    STAT_EXT = .a
    ifeq ($(UNAME_S),Linux)
        DYN_EXT = .so
    endif
    ifeq ($(UNAME_S),Darwin)
        DYN_EXT = .dylib
    endif
    ARROW = \-\>
    EXTENSION =
endif

DENSITY_SRC = $(wildcard $(SRC_DIRECTORY)*.c)
DENSITY_OBJ = $(DENSITY_SRC:.c=.o)
SPOOKYHASH_SRC = $(wildcard $(SPOOKYHASH_SRC_DIRECTORY)*.c)
SPOOKYHASH_OBJ = $(SPOOKYHASH_SRC:.c=.o)
BENCHMARK_SRC = $(wildcard $(BENCHMARK_SRC_DIRECTORY)*.c)
BENCHMARK_OBJ = $(BENCHMARK_SRC:.c=.o)
ALL_OBJ = $(DENSITY_OBJ) $(SPOOKYHASH_OBJ)

.PHONY: compile-header compile-library-header link-header link-benchmark-header

ifeq ($(COMPILER),)
all: error
library: error
else
all: $(BENCHMARK)$(EXTENSION)
library: $(DENSITY)$(DYN_EXT) $(DENSITY)$(STAT_EXT)
endif

error:
	@echo ${bold}Error${normal}
	@echo Please install one of the supported compilers (Clang, GCC).
	exit 1

%.o: %.c
	@echo $^ $(ARROW) $@
	@$(COMPILER) -c $(CFLAGS) $(FLTO) $< -o $@

compile-benchmark:
	@cd $(BENCHMARK_DIRECTORY) && $(MAKE) compile

compile-spookyhash:
	@cd $(SPOOKYHASH_DIRECTORY) && $(MAKE) compile-library

compile-density-header: compile-spookyhash
	@echo ${bold}Compiling Density${normal} using $(COMPILER) ...

compile-density: compile-density-header $(DENSITY_OBJ)
	@echo Done.
	@echo

link-density-header: compile-density
	@echo ${bold}Linking Density${normal} ...

link-benchmark-header: compile-density compile-benchmark
	@echo ${bold}Linking Benchmark${normal} ...

$(DENSITY)$(STAT_EXT): link-density-header $(ALL_OBJ)
	@echo *.o $(ARROW) ${bold}$(DENSITY)$(STAT_EXT)${normal}
	@ar rcs $(DENSITY)$(STAT_EXT) $(ALL_OBJ)
	@echo Done.
	@echo

$(DENSITY)$(DYN_EXT): link-density-header $(ALL_OBJ)
	@echo *.o $(ARROW) ${bold}$(DENSITY)$(DYN_EXT)${normal}
	@$(COMPILER) -shared $(FLTO) -o $(DENSITY)$(DYN_EXT) $(ALL_OBJ)
	@$(RM) $(DENSITY).exp
	@echo Done.
	@echo

$(BENCHMARK)$(EXTENSION): link-benchmark-header $(BENCHMARK_OBJ) $(DENSITY)$(STAT_EXT)
	@echo *.o $(ARROW) ${bold}$(BENCHMARK)$(EXTENSION)${normal}
	@$(COMPILER) -ldensity $(FLTO) -o $(BENCHMARK)$(EXTENSION) $(BENCHMARK_OBJ)
	@$(RM) $(BENCHMARK).lib
	@$(RM) $(BENCHMARK).exp
	@echo Done.
	@echo

clean-spookyhash:
	@cd $(SPOOKYHASH_DIRECTORY) && $(MAKE) clean

clean-benchmark:
	@cd $(BENCHMARK_DIRECTORY) && $(MAKE) clean

clean: clean-benchmark clean-spookyhash
	@echo ${bold}Cleaning Density objects${normal} ...
	@$(RM) $(DENSITY_OBJ)
	@$(RM) $(DENSITY)$(DYN_EXT)
	@$(RM) $(DENSITY)$(STAT_EXT)
	@echo Done.
	@echo