# ----------------------------------------------------------------------------

# Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

# Copyright (C) 2008  Embecosm Limited

# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ----------------------------------------------------------------------------

# Linux makefile for OSCI SystemC wrapper project test programs

# $Id$

# Tools

CC = or32-uclinux-gcc
CFLAGS = -ggdb
LD = or32-uclinux-ld


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
# Clean up

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) hello
	$(RM) logger_test
	$(RM) uart_loop
	$(RM) uart_loop_intr
