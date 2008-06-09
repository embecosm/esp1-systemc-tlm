/* profiler.h -- profiling utility
   Copyright (C) 2001 Marko Mlinar, markom@opencores.org

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
   by or1ksim. (use --profile option at command line, when running or1ksim.  */

#ifndef __PROFILER_H
#define __PROFILER_H

#define MAX_STACK 1024
#define MAX_FUNCS 1024

#define PROF_CUMULATIVE 0x01
#define PROF_QUIET	0x02

struct stack_struct {
  /* Function address */
  unsigned int addr;
  
  /* Cycles of function start; cycles of subfunctions are added later */
  unsigned int cycles;
  
  /* Return address */
  unsigned int raddr;
  
  /* Name of the function */
  char name[33];
};

struct func_struct {
  /* Start address of function */
  unsigned int addr;
  
  /* Name of the function */
  char name[33];
  
  /* Total cycles spent in function */
  long cum_cycles;
  
  /* Calls to this function */
  long calls;
};

extern struct func_struct prof_func[MAX_FUNCS];

/* Total number of functions */
extern int prof_nfuncs;
extern int prof_cycles;

/* Print out command line help */
void prof_help ();

/* Acquire data from profiler file */
int prof_acquire (char *fprofname);

/* Print out profiling data */
void prof_print ();

/* Set options */
void prof_set (int _quiet, int _cumulative);

int main_profiler (int argc, char *argv[]);
#endif /* not __PROFILER_H */
