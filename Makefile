#
# Density
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

recursive_wildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call recursive_wildcard,$d/,$2))

UPDATE_SUBMODULES := $(shell git submodule update --init --recursive)

TARGET = libdensity
CFLAGS = -Ofast -flto -std=c99 -Wall -MD
LFLAGS = -flto

BUILD_DIRECTORY = build
DENSITY_BUILD_DIRECTORY = $(BUILD_DIRECTORY)/density
SRC_DIRECTORY = src

TARGET_TRIPLE := $(subst -, ,$(shell $(CC) -dumpmachine))
TARGET_ARCH   := $(word 1,$(TARGET_TRIPLE))
TARGET_OS     := $(word 3,$(TARGET_TRIPLE))

ifeq ($(TARGET_OS),mingw32)
else ifeq ($(TARGET_OS),cygwin)
else
	CFLAGS += -fpic
endif

ifeq ($(ARCH),)
	ifeq ($(NATIVE),)
		ifeq ($(TARGET_ARCH),powerpc)
			CFLAGS += -mtune=native
		else ifeq ($(TARGET_ARCH),arm64)
    			CFLAGS += -mtune=native
		else
			CFLAGS += -march=native
		endif
	endif
else
	ifeq ($(ARCH),32)
		CFLAGS += -m32
		LFLAGS += -m32
	endif

	ifeq ($(ARCH),64)
		CFLAGS += -m64
		LFLAGS += -m64
	endif
endif

ifeq ($(OS),Windows_NT)
	bold =
	normal =
	ARROW = ^-^>
	EXTENSION = .dll
	BENCHMARK_EXTENSION = .exe
	SEPARATOR = \\
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
	SEPARATOR = /
	ifeq ($(shell lsb_release -a 2>/dev/null | grep Distributor | awk '{ print $$3 }'),Ubuntu)
		CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
	endif
endif
STATIC_EXTENSION = .a

DEPS=$(wildcard *.d)
DENSITY_SRC = $(call recursive_wildcard,$(SRC_DIRECTORY)/,*.c)
DENSITY_OBJ = $(patsubst $(SRC_DIRECTORY)%.c, $(DENSITY_BUILD_DIRECTORY)%.o, $(DENSITY_SRC))

.PHONY: pre-compile post-compile pre-link post-link library benchmark

all: benchmark

$(DENSITY_BUILD_DIRECTORY)/%.o: $(SRC_DIRECTORY)/%.c
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
	$(AR) crs $(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION) $(DENSITY_OBJ)
	$(CC) $(LFLAGS) -shared -o $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION) $(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION)

post-link: link
	@echo Done.
	@echo
	@echo Static library file : ${bold}$(BUILD_DIRECTORY)$(SEPARATOR)$(TARGET)$(STATIC_EXTENSION)${normal}
	@echo Dynamic library file : ${bold}$(BUILD_DIRECTORY)$(SEPARATOR)$(TARGET)$(EXTENSION)${normal}
	@echo

$(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION): post-link

library: post-link

benchmark: library
	@$(MAKE) -C benchmark/
	@echo Please type ${bold}$(BUILD_DIRECTORY)$(SEPARATOR)benchmark$(BENCHMARK_EXTENSION)${normal} to launch the benchmark binary.
	@echo

clean:
	@$(MAKE) -C benchmark/ clean
	@echo ${bold}Cleaning Density build files${normal} ...
	@rm -f $(DENSITY_OBJ)
	@rm -f $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION)
	@rm -f $(BUILD_DIRECTORY)/$(TARGET)$(STATIC_EXTENSION)
	@rm -f $(DEPS)
	@echo Done.
	@echo

-include $(DENSITY_OBJ:.o=.d)
