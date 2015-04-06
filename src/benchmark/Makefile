#
# Centaurean Density Benchmark
#
# Copyright (c) 2015, Guillaume Voirin
# All rights reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# 6/04/15 0:31
#

CFLAGS = -Ofast -fomit-frame-pointer -w -flto -std=c99

SRC_DIRECTORY = ./src/

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
endif

BENCHMARK_SRC = $(wildcard $(SRC_DIRECTORY)*.c)
BENCHMARK_OBJ = $(BENCHMARK_SRC:.c=.o)
ALL_OBJ = $(BENCHMARK_OBJ)

.PHONY: compile-header

all: compile

%.o: %.c
	@echo $^ $(ARROW) $@
	@$(CC) -c $(CFLAGS) $< -o $@

compile-header:
	@echo ${bold}Compiling Benchmark${normal} ...

compile: compile-header $(BENCHMARK_OBJ)
	@echo Done.
	@echo

clean:
	@echo ${bold}Cleaning Benchmark objects${normal} ...
	@rm -f $(BENCHMARK_OBJ)
	@echo Done.
	@echo