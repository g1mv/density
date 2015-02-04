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
# 4/02/15 22:02
#

ifeq ($(OS),Windows_NT)
    bold =
    normal =
else
    bold = `tput bold`
    normal = `tput sgr0`
endif

TARGET = density
SRC_DIRECTORY = ./src/
SPOOKYHASH_SRC_DIRECTORY = ./src/spookyhash/src/

ifeq ($(OS),Windows_NT)
    DYN_EXT = .dll
    STAT_EXT = .lib
    ARROW = ^-^>
else
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

SRC = $(wildcard $(SRC_DIRECTORY)*.c $(SPOOKYHASH_SRC_DIRECTORY)*.c)
OBJ = $(SRC:.c=.o)

ALL_OBJ = $(OBJ)

.PHONY: link-header link

all: compile link-header link

$(SPOOKYHASH_SRC_DIRECTORY)Makefile:
	@echo ${bold}Cloning SpookyHash${normal} ...
	@git submodule init
	@git submodule update
	@echo

link-header:
	@echo ${bold}Linking Density${normal} ...

compile: $(SPOOKYHASH_SRC_DIRECTORY)Makefile
	@cd $(SPOOKYHASH_SRC_DIRECTORY) && $(MAKE) EXTRA_CFLAGS=-fPIC compile
	@cd $(SRC_DIRECTORY) && $(MAKE) EXTRA_CFLAGS=-fPIC compile

link: compile link-header $(TARGET)$(STAT_EXT) $(TARGET)$(DYN_EXT)
	@echo Done.
	@echo

$(TARGET)$(DYN_EXT): $(ALL_OBJ)
	@echo $^ $(ARROW) ${bold}$(TARGET)$(DYN_EXT)${normal}
	@$(CC) -shared -o $(TARGET)$(DYN_EXT) $^

$(TARGET)$(STAT_EXT): $(ALL_OBJ)
	@echo $^ $(ARROW) ${bold}$(TARGET)$(STAT_EXT)${normal}
	@ar r $(TARGET)$(STAT_EXT) $^
	@ranlib $(TARGET)$(STAT_EXT)

clean:
	@rm -f $(TARGET)$(DYN_EXT)
	@rm -f $(TARGET)$(STAT_EXT)
	@cd $(SPOOKYHASH_SRC_DIRECTORY) && $(MAKE) clean
	@cd $(SRC_DIRECTORY) && $(MAKE) clean