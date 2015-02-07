#
# Centaurean Density
#
# Copyright (c) 2013, Guillaume Voirin
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
# 19/10/13 00:27
#

UPDATE_SUBMODULES := $(shell git submodule update --init --recursive)

TARGET = density
CFLAGS = -O3 -w -flto -std=c99

SRC_DIRECTORY = ./src/
SPOOKYHASH_DIRECTORY = ./src/spookyhash/
SPOOKYHASH_SRC_DIRECTORY = $(SPOOKYHASH_DIRECTORY)src/

ifeq ($(OS),Windows_NT)
    bold =
    normal =
    DYN_EXT = .dll
    STAT_EXT = .lib
    ARROW = ^-^>
else
    bold = `tput bold`
    normal = `tput sgr0`
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        DYN_EXT = .so
    endif
    ifeq ($(UNAME_S),Darwin)
        DYN_EXT = .dylib
    endif
    STAT_EXT = .a
    ARROW = \-\>
endif

DENSITY_SRC = $(wildcard $(SRC_DIRECTORY)*.c)
DENSITY_OBJ = $(DENSITY_SRC:.c=.o)
OTHER_SRC = $(wildcard $(SPOOKYHASH_SRC_DIRECTORY)*.c)
OTHER_OBJ = $(OTHER_SRC:.c=.o)
ALL_OBJ = $(DENSITY_OBJ) $(OTHER_OBJ)

.PHONY: compile-header compile-library-header link-header

all: $(TARGET)$(DYN_EXT) $(TARGET)$(STAT_EXT)

%.o: %.c
	@echo $^ $(ARROW) $@
	@$(CC) -c $(CFLAGS) $< -o $@

compile-submodules:
	@cd $(SPOOKYHASH_DIRECTORY) && $(MAKE) compile

compile-header: compile-submodules
	@echo ${bold}Compiling Density${normal} ...

compile-library-header:
	@$(eval CFLAGS = $(CFLAGS) -fPIC)
	@echo ${bold}Compiling Density as a library${normal} ...

compile: compile-header $(DENSITY_OBJ)
	@echo Done.
	@echo

compile-library: compile-library-header $(DENSITY_OBJ)
	@echo Done.
	@echo

link-header: compile-library
	@echo ${bold}Linking Density${normal} ...

$(TARGET)$(DYN_EXT): link-header $(ALL_OBJ)
	@echo *.o $(ARROW) ${bold}$(TARGET)$(DYN_EXT)${normal}
	@$(CC) -shared -o $(TARGET)$(DYN_EXT) $(ALL_OBJ)
	@echo Done.
	@echo

$(TARGET)$(STAT_EXT): link-header $(ALL_OBJ)
	@echo *.o $(ARROW) ${bold}$(TARGET)$(STAT_EXT)${normal}
	@ar r $(TARGET)$(STAT_EXT) $(ALL_OBJ)
	@ranlib $(TARGET)$(STAT_EXT)
	@echo Done.
	@echo

clean-submodules:
	@cd $(SPOOKYHASH_DIRECTORY) && $(MAKE) clean

clean: clean-submodules
	@echo ${bold}Cleaning Density objects${normal} ...
	@rm -f $(DENSITY_OBJ)
	@rm -f $(TARGET)$(DYN_EXT)
	@rm -f $(TARGET)$(STAT_EXT)
	@echo Done.
	@echo