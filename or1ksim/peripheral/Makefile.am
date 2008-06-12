# Makefile -- Makefile for peripherals simulation
# Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
# Copyright (C) 2008 Jeremy Bennett, jeremy@jeremybennett.com
#
# This file is part of OpenRISC 1000 Architectural Simulator.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 675 Mass
# Ave, Cambridge, MA 02139, USA.
#

SUBDIRS = channels

noinst_LTLIBRARIES            = libperipheral.la
libperipheral_la_SOURCES      = generic.c \
				16450.c \
				dma.c \
				mc.c \
				eth.c \
				crc32.c \
				gpio.c \
				vga.c \
				fb.c \
				ps2kbd.c \
				atahost.c \
				atadevice.c \
				atadevice_cmdi.c \
				memory.c
