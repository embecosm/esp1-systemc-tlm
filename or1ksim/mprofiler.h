/* mprofiler.c -- memory profiling utility
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

/* Command line utility, that displays profiling information, generated
   by or1ksim. (use --mprofile option at command line, when running or1ksim.  */

#ifndef __MPROFILER_H
#define __MPROFILER_H

/* output modes */
#define MODE_DETAIL     0
#define MODE_PRETTY     1
#define MODE_ACCESS     2
#define MODE_WIDTH      3

/* Input buffer size */
#define BUF_SIZE        256

/* HASH */
#define HASH_SIZE       0x10000
#define HASH_FUNC(x)    ((x) & 0xffff)

int main_mprofiler (int argc, char *argv[]);
void mp_help(void);

#endif /* not __MPROFILER_H */
