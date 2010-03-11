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
all: hello logger_test uart_loop uart_loop_intr


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

logger_test: start.o utils.o logger_test.o 
	$(LD) -Ttext 0x0 $^ -o $@

logger_test.o: logger_test.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# UART loop test. Ensure start.o is first!

uart_loop: start.o utils.o uart_loop.o 
	$(LD) -Ttext 0x0 $^ -o $@

uart_loop.o: uart_loop.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# UART loop test with interrupts. Ensure start.o is first!

uart_loop_intr: start.o utils.o uart_loop_intr.o 
	$(LD) -Ttext 0x0 $^ -o $@

uart_loop_intr.o: uart_loop_intr.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Documentation

doc: doxygen.config mainpage start.s utils.h utils.c bitutils.c hello.c \
     logger_test.c uart_loop.c uart_loop_intr.c
	doxygen doxygen.config

# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM)    *.o
	$(RM) -r doc
	$(RM)    hello
	$(RM)    logger_test
	$(RM)    uart_loop
	$(RM)    uart_loop_intr
