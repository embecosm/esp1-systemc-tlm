/* profiler.c -- profiling utility
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

#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "profiler.h"
#include "sim-config.h"

static struct stack_struct stack[MAX_STACK];

struct func_struct prof_func[MAX_FUNCS];

/* Total number of functions */
int prof_nfuncs = 0;

/* Current depth */
static int nstack = 0;

/* Max depth */
static int maxstack = 0;

/* Number of total calls */
static int ntotcalls = 0;

/* Number of covered calls */
static int nfunccalls = 0;

/* Current cycles */
int prof_cycles = 0;

/* Whether we are in cumulative mode */
static int cumulative = 0;

/* Whether we should not report warnings */
static int quiet = 0;

/* File to read from */
static FILE *fprof = 0;

/* Print out command line help */
void prof_help ()
{
  PRINTF ("profiler [-c] [-q] -g [profile_file_name]\n");
  PRINTF ("\t-c\t--cumulative\t\tcumulative sum of cycles in functions\n");
  PRINTF ("\t-q\t--quiet\t\t\tsuppress messages\n");
  PRINTF ("\t-g\t--generate [profile_file_name]\n");
  PRINTF ("\t\t\t\t\toutput profiling results to\n");
  PRINTF ("\t\t\t\t\tstdout/profile_file_name\n");
}

/* Acquire data from profiler file */
int prof_acquire (char *fprofname)
{
  int line = 0;
  int reopened = 0;
  
  if (runtime.sim.fprof) {
    fprof = runtime.sim.fprof;
    reopened = 1;
    rewind (fprof);
  } else fprof = fopen (fprofname, "rt");
  
  if (!fprof) {
    fprintf (stderr, "Cannot open profile file: %s\n", fprofname);
    return 1;
  }
  
  while (1) {
    char dir = fgetc (fprof);
    line++;
    if (dir == '+') {
      if (fscanf (fprof, "%08X %08X %08X %s\n", &stack[nstack].cycles, &stack[nstack].raddr,
                  &stack[nstack].addr, &stack[nstack].name[0]) != 4)
        fprintf (stderr, "Error reading line #%i\n", line);
      else {
        prof_cycles = stack[nstack].cycles;
        nstack++;
        if (nstack > maxstack)
          maxstack = nstack;
      }
      ntotcalls++;
    } else if (dir == '-') {
      struct stack_struct s;
      if (fscanf (fprof, "%08X %08X\n", &s.cycles, &s.raddr) != 2)
        fprintf (stderr, "Error reading line #%i\n", line);
      else {
        int i;
        prof_cycles = s.cycles;
        for (i = nstack - 1; i >= 0; i--)
          if (stack[i].raddr == s.raddr) break;
        if (i >= 0) {
          /* pop everything above current from stack,
             if more than one, something went wrong */
          while (nstack > i) {
            int j;
            long time;
            nstack--;
            time = s.cycles - stack[nstack].cycles;
            if (!quiet && time < 0) {
              fprintf (stderr, "WARNING: Negative time at %s (return addr = %08X).\n", stack[i].name, stack[i].raddr);
              time = 0;
            }
            
            /* Whether in normal mode, we must substract called function from execution time.  */
            if (!cumulative)
              for (j = 0; j < nstack; j++)
                stack[j].cycles += time;

            if (!quiet && i != nstack)
              fprintf (stderr, "WARNING: Missaligned return call for %s (%08X) (found %s @ %08X), closing.\n", stack[nstack].name, stack[nstack].raddr, stack[i].name, stack[i].raddr);
            
            for (j = 0; j < prof_nfuncs; j++)
              if (stack[nstack].addr == prof_func[j].addr) { /* function exists, append. */
                prof_func[j].cum_cycles += time;
                prof_func[j].calls++;
                nfunccalls++;
                break;
              }
            if (j >= prof_nfuncs) { /* function does not yet exist, create new. */
              prof_func[prof_nfuncs].cum_cycles = time;
              prof_func[prof_nfuncs].calls = 1;
              nfunccalls++;
              prof_func[prof_nfuncs].addr = stack[nstack].addr;
              strcpy (prof_func[prof_nfuncs].name, stack[nstack].name);
              prof_nfuncs++;
            }
          }
        } else if (!quiet) fprintf (stderr, "WARNING: Cannot find return call for (%08X), ignoring.\n", s.raddr);
      }
    } else
      break;
  }
  
  /* If we have reopened the file, we need to add end of "[outside functions]" */
  if (reopened) {
    prof_cycles = runtime.sim.cycles;
    /* pop everything above current from stack,
       if more than one, something went wrong */
    while (nstack > 0) {
      int j;
      long time;
      nstack--;
      time = runtime.sim.cycles - stack[nstack].cycles;
      /* Whether in normal mode, we must substract called function from execution time.  */
      if (!cumulative)
        for (j = 0; j < nstack; j++) stack[j].cycles += time;

      for (j = 0; j < prof_nfuncs; j++)
        if (stack[nstack].addr == prof_func[j].addr) { /* function exists, append. */
          prof_func[j].cum_cycles += time;
          prof_func[j].calls++;
          nfunccalls++;
          break;
        }
      if (j >= prof_nfuncs) { /* function does not yet exist, create new. */
        prof_func[prof_nfuncs].cum_cycles = time;
        prof_func[prof_nfuncs].calls = 1;
        nfunccalls++;
        prof_func[prof_nfuncs].addr = stack[nstack].addr;
        strcpy (prof_func[prof_nfuncs].name, stack[nstack].name);
        prof_nfuncs++;
      }
    }
  } else fclose(fprof);
  return 0;
}

/* Print out profiling data */
void prof_print ()
{
  int i, j;
  if (cumulative)
    PRINTF ("CUMULATIVE TIMES\n");
  PRINTF ("---------------------------------------------------------------------------\n");
  PRINTF ("|function name            |addr    |# calls |avg cycles  |total cyles     |\n");
  PRINTF ("|-------------------------+--------+--------+------------+----------------|\n");
  for (j = 0; j < prof_nfuncs; j++) {
    int bestcyc = 0, besti = 0;
    for (i = 0; i < prof_nfuncs; i++)
      if (prof_func[i].cum_cycles > bestcyc) {
        bestcyc = prof_func[i].cum_cycles;
        besti = i;
      }
    i = besti;
    PRINTF ("| %-24s|%08X|%8li|%12.1f|%11li,%3.0f%%|\n",
            prof_func[i].name, prof_func[i].addr, prof_func[i].calls, ((double)prof_func[i].cum_cycles / prof_func[i].calls), prof_func[i].cum_cycles, (100. * prof_func[i].cum_cycles / prof_cycles));
    prof_func[i].cum_cycles = -1;
  }
  PRINTF ("---------------------------------------------------------------------------\n");
  PRINTF ("Total %i functions, %i cycles.\n", prof_nfuncs, prof_cycles);
  PRINTF ("Total function calls %i/%i (max depth %i).\n", nfunccalls, ntotcalls, maxstack);
}

/* Set options */
void prof_set (int _quiet, int _cumulative)
{
  quiet = _quiet;
  cumulative = _cumulative;
}

int main_profiler (int argc, char *argv[]) {
  char fprofname[50] = "sim.profile";

  if (argc > 4 || argc < 2) {
    prof_help ();
    return 1;
  }

  argv++; argc--;
  while (argc > 0) {
    if (!strcmp(argv[0], "-q") || !strcmp(argv[0], "--quiet")) {
      quiet = 1;
      argv++; argc--;
    } else if (!strcmp(argv[0], "-c") || !strcmp(argv[0], "--cumulative")) {
      cumulative = 1;
      argv++; argc--;
    } else if (strcmp(argv[0], "-g") && strcmp(argv[0], "--generate")) {
      prof_help ();
      return -1;
    } else {
      argv++; argc--;
      if (argv[0] && argv[0][0] != '-') {
        strcpy (&fprofname[0], argv[0]);
        argv++; argc--;
      }
    }
  }

  prof_acquire (fprofname);

  /* Now we have all data acquired. Print out. */
  prof_print ();
  return 0;
}
