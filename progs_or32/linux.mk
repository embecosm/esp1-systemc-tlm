# ----------------------------------------------------------------------------

#                  CONFIDENTIAL AND PROPRIETARY INFORMATION
#                  ========================================

# Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.

# This file contains confidential and proprietary information of Embecosm and
# is protected by copyright, trade secret and other regional, national and
# international laws, and may be embodied in patents issued or pending.

# Receipt or possession of this file does not convey any rights to use,
# reproduce, disclose its contents, or to manufacture, or sell anything it may
# describe.

# Reproduction, disclosure or use without specific written authorization of
# Embecosm is strictly forbidden.

# Reverse engineering is prohibited.

# ----------------------------------------------------------------------------

# Linux makefile for OSCI SystemC wrapper project example programs

# $Id$

# Tools

CC = or32-uclinux-gcc
CFLAGS = -ggdb
LD = or32-uclinux-ld


# ----------------------------------------------------------------------------
# Make the lot

.PHONY: all
all: hello generic_io uart_loop


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

generic_io: start.o utils.o generic_io.o 
	$(LD) -Ttext 0x0 $^ -o $@

generic_io.o: generic_io.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Generic I/O. Ensure start.o is first!

uart_loop: start.o utils.o uart_loop.o 
	$(LD) -Ttext 0x0 $^ -o $@

uart_loop.o: uart_loop.c
	$(CC) $(CFLAGS) -c $<


# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) hello
	$(RM) generic_io
	$(RM) uart_loop
