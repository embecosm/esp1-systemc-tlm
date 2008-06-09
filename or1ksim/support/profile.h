/* profile.c -- definitions for profiling
   Copyright (C) 2002 Marko Mlinar, markom@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#define MPROF_READ     1
#define MPROF_WRITE    2
#define MPROF_FETCH    4
#define MPROF_8        8
#define MPROF_16       16
#define MPROF_32       32

/* Adds a new entry to the memory profile file */
void mprofile (oraddr_t memaddr, unsigned char type);

struct mprofentry_struct {
  oraddr_t addr;
  unsigned char type;
};
