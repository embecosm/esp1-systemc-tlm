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
LD = or32-uclinux-ld


# ----------------------------------------------------------------------------
# Make the lot

.PHONY: all
all: hello generic_io


# ----------------------------------------------------------------------------
# Bootloader - puts code at 0x100

bootloader.o: bootloader.s
	$(CC) -c $<


# ----------------------------------------------------------------------------
# Hello world. Ensure bootloader.o is first!

hello: bootloader.o hello.o 
	$(LD) -Ttext 0x0 $^ -o $@

hello.o: hello.c
	$(CC) -c $<


# ----------------------------------------------------------------------------
# Generic I/O. Ensure bootloader.o is first!

generic_io: bootloader.o generic_io.o 
	$(LD) -Ttext 0x0 $^ -o $@

generic_io.o: generic_io.c
	$(CC) -c $<


# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) hello
	$(RM) generic_io
