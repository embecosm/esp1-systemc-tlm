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

# Linux makefile for OSCI SystemC wrapper project simple test program

# $Id$

# Tools

CC = gcc
LD = ld

CFLAGS += -ggdb

# Where stuff is

LIBDIR = /opt/or1ksim/lib

# ----------------------------------------------------------------------------
# Make the lot

.PHONY: all
all: hello-sim


# ----------------------------------------------------------------------------
# Trivial use of the simulation library

hello-sim: hello-sim.o 
	$(CC) $(CFLAGS) $< -Wl,--rpath -Wl,$(LIBDIR) -L$(LIBDIR) -lsim -o $@

hello-sim.o: hello-sim.c
	$(CC) $(CFLAGS) -I/opt/or1ksim/include -c $<


# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) hello-sim
