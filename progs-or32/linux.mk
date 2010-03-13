# Top level Linux makefile for OpenRISC test programs.

# Copyright (C) 2009 Embecosm Limited

# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

# This file is part of the example programs for "Building a Loosely Timed SoC
# Model with OSCI TLM 2.0"

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http:#www.gnu.org/licenses/>.  */

# -----------------------------------------------------------------------------
# This code is commented throughout for use with Doxygen.
# -----------------------------------------------------------------------------

# Tools

CC = or32-elf-gcc
CFLAGS = -ggdb
LD = or32-elf-ld


# ----------------------------------------------------------------------------
# Make the lot

.PHONY: all
all: hello logger-test uart-loop uart-loop-intr


# ----------------------------------------------------------------------------
# Bootloader - puts code at 0x100

start.o: start.s
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Utilities

utils.o: utils.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Hello world. Ensure start.o is first!

hello: start.o utils.o hello.o 
	$(LD) -Ttext 0x0 $^ -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Generic I/O. Ensure start.o is first!

logger-test: start.o utils.o logger-test.o 
	$(LD) -Ttext 0x0 $^ -o $@

logger-test.o: logger-test.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# UART loop test. Ensure start.o is first!

uart-loop: start.o utils.o uart-loop.o 
	$(LD) -Ttext 0x0 $^ -o $@

uart-loop.o: uart-loop.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# UART loop test with interrupts. Ensure start.o is first!

uart-loop-intr: start.o utils.o uart-loop-intr.o 
	$(LD) -Ttext 0x0 $^ -o $@

uart-loop-intr.o: uart-loop-intr.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Documentation

doc: doxygen.config mainpage start.s utils.h utils.c bitutils.c hello.c \
     logger-test.c uart-loop.c uart-loop-intr.c
	doxygen doxygen.config

# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM)    *.o
	$(RM) -r doc
	$(RM)    hello
	$(RM)    logger-test
	$(RM)    uart-loop
	$(RM)    uart-loop-intr
