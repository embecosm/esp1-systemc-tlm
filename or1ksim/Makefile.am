# Makefile.am -- Top level automake configuration file
#  Copyright (C) 2000 Jeremy Bennett (jeremy@jeremybennett.com)
#  Copyright (C) 1999-2008 OpenCores

# This file is part of OpenRISC 1000 Architectural Simulator.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 675 Mass
# Ave, Cambridge, MA 02139, USA.

# $Id$


SUBDIRS               = cpu \
			bpb \
			support \
			cache \
			mmu \
			peripheral \
			tick \
			pm \
			pic \
			debug \
			vapi \
			cuc \
			port

# Build a standalone simulator and a linkable library of the simulator

bin_PROGRAMS	      = sim
lib_LTLIBRARIES       = libsim.la
include_HEADERS       = or1ksim.h

# The internal library

noinst_LTLIBRARIES    = libor1ksys.la

libor1ksys_la_SOURCES = toplevel_support.c \
			sim-config.c \
			sim-config.h \
			profiler.c \
			mprofiler.c \
			profiler.h \
			mprofiler.h \
			sim-cmd.c

libor1ksys_la_LIBADD  = cpu/common/libcommon.la \
			cpu/$(CPU_ARCH)/libarch.la \
			cpu/or1k/libor1k.la \
			support/libsupport.la \
			mmu/libmmu.la \
			bpb/libbpb.la \
			cache/libcache.la \
			peripheral/libperipheral.la \
			peripheral/channels/libchannels.la \
			tick/libtick.la \
			pm/libpm.la \
			pic/libpic.la \
			debug/libdebug.la \
			vapi/libvapi.la \
			cuc/libcuc.la \
			port/libport.la

libor1ksys_la_LDFLAGS = -lm

# Standalone program

sim_SOURCES 	      = toplevel.c
sim_LDADD   	      = libor1ksys.la

# Library

libsim_la_SOURCES     = libtoplevel.c \
			or1ksim.h
libsim_la_LIBADD      = libor1ksys.la
