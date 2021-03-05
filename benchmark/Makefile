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
# 02/02/18 01:55
#

recursive_wildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call recursive_wildcard,$d/,$2))

UPDATE_SUBMODULES := $(shell git submodule update --init --recursive)

TARGET = benchmark
CFLAGS = -Ofast -flto -std=c99 -Wall -MD
LFLAGS = -flto

DEPS=$(wildcard *.d)
BUILD_DIRECTORY = ../build
DENSITY_STATIC_LIBRARY = libdensity.a
CLIENT_SRC_DIRECTORY = src
CPUTIME_SRC_DIRECTORY = libs/cputime/src
SPOOKYHASH_SRC_DIRECTORY = libs/spookyhash/src
CLIENT_BUILD_DIRECTORY = $(BUILD_DIRECTORY)/bench
CPUTIME_BUILD_DIRECTORY = $(BUILD_DIRECTORY)/cputime
SPOOKYHASH_BUILD_DIRECTORY = $(BUILD_DIRECTORY)/spookyhash

TARGET_TRIPLE := $(subst -, ,$(shell $(CC) -dumpmachine))
TARGET_ARCH   := $(word 1,$(TARGET_TRIPLE))
TARGET_OS     := $(word 3,$(TARGET_TRIPLE))

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
	EXTENSION = .exe
else
	bold = `tput bold`
	normal = `tput sgr0`
	ARROW = \-\>
	EXTENSION =
	ifeq ($(shell lsb_release -a 2>/dev/null | grep Distributor | awk '{ print $$3 }'),Ubuntu)
		CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
	endif
endif

CLIENT_SRC = $(call recursive_wildcard,$(CLIENT_SRC_DIRECTORY)/,*.c)
CPUTIME_SRC = $(call recursive_wildcard,$(CPUTIME_SRC_DIRECTORY)/,*.c)
SPOOKYHASH_SRC = $(call recursive_wildcard,$(SPOOKYHASH_SRC_DIRECTORY)/,*.c)
CLIENT_OBJ = $(patsubst $(CLIENT_SRC_DIRECTORY)%.c, $(CLIENT_BUILD_DIRECTORY)%.o, $(CLIENT_SRC))
CPUTIME_OBJ = $(patsubst $(CPUTIME_SRC_DIRECTORY)%.c, $(CPUTIME_BUILD_DIRECTORY)%.o, $(CPUTIME_SRC))
SPOOKYHASH_OBJ = $(patsubst $(SPOOKYHASH_SRC_DIRECTORY)%.c, $(SPOOKYHASH_BUILD_DIRECTORY)%.o, $(SPOOKYHASH_SRC))

BENCHMARK_SRC = $(CLIENT_SRC) $(CPUTIME_SRC) $(SPOOKYHASH_SRC)
BENCHMARK_OBJ = $(CLIENT_OBJ) $(CPUTIME_OBJ) $(SPOOKYHASH_OBJ)

.PHONY: pre-compile post-compile pre-link post-link

all: $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION)

$(CLIENT_BUILD_DIRECTORY)/%.o: $(CLIENT_SRC_DIRECTORY)/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

$(CPUTIME_BUILD_DIRECTORY)/%.o: $(CPUTIME_SRC_DIRECTORY)/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

$(SPOOKYHASH_BUILD_DIRECTORY)/%.o: $(SPOOKYHASH_SRC_DIRECTORY)/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

pre-compile:
	@echo ${bold}Compiling Density benchmark${normal} ...

compile: pre-compile $(BENCHMARK_OBJ)

post-compile: compile
	@echo Done.
	@echo

pre-link : post-compile
	@echo ${bold}Linking Density benchmark${normal} ...

link: pre-link $(BENCHMARK_OBJ)
	$(CC) $(LFLAGS) -o $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION) $(BENCHMARK_OBJ) $(BUILD_DIRECTORY)/$(DENSITY_STATIC_LIBRARY)

post-link: link
	@echo Done.
	@echo

$(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION): post-link

clean:
	@echo ${bold}Cleaning Density benchmark build files${normal} ...
	@rm -f $(BENCHMARK_OBJ)
	@rm -f $(BUILD_DIRECTORY)/$(TARGET)$(EXTENSION)
	@rm -f $(DEPS)
	@echo Done.
	@echo

-include $(BENCHMARK_OBJ:.o=.d)
