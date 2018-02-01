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

TARGET = benchmark
CFLAGS = -Ofast -fomit-frame-pointer -flto -std=c99 -Wall

SRC_DIRECTORY = ./src/
OBJ_DIRECTORY = ./build/

ifeq ($(OS),Windows_NT)
    bold =
    normal =
    EXTENSION = .exe
    ARROW = ^-^>
else
    bold = `tput bold`
    normal = `tput sgr0`
    EXTENSION =
    ARROW = \-\>
endif

DENSITY_SRC = $(wildcard $(SRC_DIRECTORY)*.c)
DENSITY_OBJ_STRIPPED = $(DENSITY_SRC:.c=.o)
DENSITY_OBJPATH := $(addprefix $(OBJ_DIRECTORY),$(notdir $(DENSITY_OBJ_STRIPPED)))
DENSITY_OBJ = $(DENSITY_OBJPATH)
ALL_OBJ = $(DENSITY_OBJ)

.PHONY: compile-header link-header

all: $(TARGET)$(EXTENSION)

%.o: %.c
	@echo $^ $(ARROW) $@
	@$(CC) -c $(CFLAGS) $< -o $@

compile-header:
	@echo ${bold}Compiling Density${normal} ...

compile: compile-header $(DENSITY_OBJ)
	@echo Done.
	@echo

link-header: compile
	@echo ${bold}Linking Density${normal} ...

$(TARGET)$(EXTENSION): link-header $(ALL_OBJ)
	@echo *.o $(ARROW) $(TARGET)$(EXTENSION)
	@$(CC) -o $(TARGET)$(EXTENSION) $(ALL_OBJ)
	@echo Done.
	@echo

clean:
	@echo ${bold}Cleaning Density build files${normal} ...
	@rm -f $(DENSITY_OBJ)
	@rm -f $(TARGET)$(EXTENSION)
	@echo Done.
	@echo
