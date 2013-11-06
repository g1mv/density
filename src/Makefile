#
# Centaurean libssc
# http://www.libssc.net
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

ifeq ($(OS),Windows_NT)
    bold =
    normal =
else
    bold = `tput bold`
    normal = `tput sgr0`
endif

TARGET = libssc

CFLAGS = -std=c99 -fPIC

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

ifdef ARCH
    CFLAGS += -m$(ARCH)
endif

ifndef WARNINGS
    CFLAGS += -w
else
    CFLAGS += -Wall
endif

ifndef DEBUG
    CFLAGS += -O4
else
    CFLAGS += -O0 -g
endif

CC = @$(PREFIX)cc

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

ALL_OBJ = $(OBJ)

.PHONY: compile-header compile link-header link

all: compile-header compile link-header link

compile-header:
	@echo ${bold}Compiling libssc${normal} ...

link-header:
	@echo ${bold}Linking libssc${normal} ...

compile: compile-header $(ALL_OBJ)
	@echo Done.
	@echo

link: compile link-header $(TARGET)$(STAT_EXT) $(TARGET)$(DYN_EXT)
	@echo Done.
	@echo

$(TARGET)$(DYN_EXT): $(ALL_OBJ)
	@echo $^ $(ARROW) ${bold}$(TARGET)$(DYN_EXT)${normal}
	$(CC) -shared -o $(TARGET)$(DYN_EXT) $^ $(CFLAGS)

$(TARGET)$(STAT_EXT): $(ALL_OBJ)
	@echo $^ $(ARROW) ${bold}$(TARGET)$(STAT_EXT)${normal}
	@ar r $(TARGET)$(STAT_EXT) $^
	@ranlib $(TARGET)$(STAT_EXT)

%.o: %.c
	@echo $^ $(ARROW) $@
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	@echo ${bold}Cleaning libssc build directories${normal} ...
	@rm -f $(ALL_OBJ)
	@rm -f $(TARGET)$(DYN_EXT)
	@echo Done.
	@echo
