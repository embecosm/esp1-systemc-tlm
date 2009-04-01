/* toplevel.c -- Top level simulator support source file
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
   Copyright (C) 2008 Jeremy Bennett, jeremy@jeremybennett.com

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 675 Mass
Ave, Cambridge, MA 02139, USA. */

/* Simulator top level support routines called by the main program. Simulator
 * commands. Help and version output. SIGINT processing.  Stdout redirection is
 * specific to linux (I need to fix this).
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "parse.h"
#include "abstract.h"
#include "labels.h"
#include "sim-config.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "vapi.h"
#include "gdbcomm.h"
#include "debug_unit.h"
#include "coff.h"
#include "sched.h"
#include "profiler.h"
#include "mprofiler.h"
#include "pm.h"
#include "pic.h"
#include "stats.h"
#include "immu.h"
#include "dmmu.h"
#include "dcache_model.h"
#include "icache_model.h"
#include "branch_predict.h"
#include "dumpverilog.h"
#include "trace.h"
#include "cuc.h"
#include "tick.h"

const char *or1ksim_ver = "0.2.0";

inline void debug(int level, const char *format, ...)
{
  char *p;
  va_list ap;

  if (config.sim.debug >= level) {
    if ((p = malloc(1000)) == NULL)
      return;
    va_start(ap, format);
    (void) vsnprintf(p, 1000, format, ap);
    va_end(ap);
    PRINTF("%s", p);
    fflush(stdout);
    free(p);
  } else {
#if DEBUG
  if ((p = malloc(1000)) == NULL)
    return;
  va_start(ap, format);
  (void) vsnprintf(p, 1000, format, ap);
  va_end(ap);
  PRINTF("%s\n", p);
  fflush(stdout);
  free(p);
#endif
  }
}

void  ctrl_c( int  signum )
{
  /* Incase the user pressed ctrl+c twice without the sim reacting kill it.
   * This is incase the sim locks up in a high level routine, without executeing
   * any (or) code */
  if(runtime.sim.iprompt && !runtime.sim.iprompt_run)
    sim_done();
  runtime.sim.iprompt = 1;
  signal(SIGINT, ctrl_c);
}

/* Periodically checks runtime.sim.iprompt to see if ctrl_c has been pressed */
void check_int(void *dat)
{
  if(runtime.sim.iprompt) {
    set_stall_state (0);
    handle_sim_command();
  }
  SCHED_ADD(check_int, NULL, CHECK_INT_TIME);
}

struct sim_reset_hook {
  void *dat;
  void (*reset_hook)(void *);
  struct sim_reset_hook *next;
};

static struct sim_reset_hook *sim_reset_hooks = NULL;

/* Registers a new reset hook, called when sim_reset below is called */
void reg_sim_reset(void (*reset_hook)(void *), void *dat)
{
  struct sim_reset_hook *new = malloc(sizeof(struct sim_reset_hook));

  if(!new) {
    fprintf(stderr, "reg_sim_reset: Out-of-memory\n");
    exit(1);
  }

  new->dat = dat;
  new->reset_hook = reset_hook;
  new->next = sim_reset_hooks;
  sim_reset_hooks = new;
}

/* Resets all subunits */
void sim_reset (void)
{
  struct sim_reset_hook *cur_reset = sim_reset_hooks;

  /* We absolutely MUST reset the scheduler first */
  sched_reset();

  while(cur_reset) {
    cur_reset->reset_hook(cur_reset->dat);
    cur_reset = cur_reset->next;
  }

  tick_reset();
  pm_reset();
  pic_reset();
  du_reset ();

  /* Make sure that runtime.sim.iprompt is the first thing to get checked */
  SCHED_ADD(check_int, NULL, 1);

  /* FIXME: Lame-ass way to get runtime.sim.mem_cycles not going into overly
   * negative numbers.  This happens because parse.c uses setsim_mem32 to load
   * the program but set_mem32 calls dc_simulate_write, which inturn calls
   * setsim_mem32.  This mess of memory statistics needs to be sorted out for
   * good one day */
  runtime.sim.mem_cycles = 0;
  cpu_reset();
}

/* Initalizes all devices and sim */
void sim_init (void)
{
  init_dmmu();
  init_immu();
  init_labels();
  init_breakpoints();
  initstats();
  build_automata();

#if DYNAMIC_EXECUTION
  /* Note: This must be called before the scheduler is used */
  init_dyn_recomp();
#endif

  sched_init();
  
  if (config.sim.profile) {
    runtime.sim.fprof = fopen(config.sim.prof_fn, "wt+");
    if(!runtime.sim.fprof) {
      fprintf(stderr, "ERROR: Problems opening profile file.\n");
      exit (1);
    } else
      fprintf(runtime.sim.fprof, "+00000000 FFFFFFFF FFFFFFFF [outside_functions]\n");
  }
  
  if (config.sim.mprofile) {
    runtime.sim.fmprof = fopen(config.sim.mprof_fn, "wb+");
    if(!runtime.sim.fmprof) {
      fprintf(stderr, "ERROR: Problems opening memory profile file.\n");
      exit (1);
    }
  }
  
  if (config.sim.exe_log) {
    runtime.sim.fexe_log = fopen(config.sim.exe_log_fn, "wt+");
    if(!runtime.sim.fexe_log) {
      PRINTF("ERROR: Problems opening exe_log file.\n");
      exit (1);
    }
  }

  if(runtime.sim.filename) {
    unsigned long endaddr = 0xFFFFFFFF;
    endaddr = loadcode(runtime.sim.filename, 0, 0); /* MM170901 always load at address zero.  */
    if (endaddr == -1) {
      fprintf(stderr, "Problems loading boot code.\n");
      exit(1);
    }
  }

  /* Disable gdb debugging, if debug module is not available.  */
  if (config.debug.gdb_enabled && !config.debug.enabled) {
    config.debug.gdb_enabled = 0;
    if (config.sim.verbose)
      fprintf (stderr, "WARNING: Debug module not enabled, cannot start gdb.\n");
  }

  if (config.debug.gdb_enabled)
    gdbcomm_init ();

  /* Enable dependency stats, if we want to do history analisis */
  if (config.sim.history && !config.cpu.dependstats) {
    config.cpu.dependstats = 1;
    if (config.sim.verbose)
      fprintf (stderr, "WARNING: dependstats stats must be enabled to do history analisis.\n");
  }

  /* Debug forces verbose */
  if (config.sim.debug && !config.sim.verbose) {
    config.sim.verbose = 1;
    fprintf (stderr, "WARNING: verbose turned on.\n");
  }
  
  /* Start VAPI before device initialization.  */  
  if (config.vapi.enabled) {
    runtime.vapi.enabled = 1;
    vapi_init ();
    if (config.sim.verbose)
      PRINTF ("VAPI started, waiting for clients.\n");
  }

  sim_reset ();
    
  /* Wait till all test are connected.  */
  if (runtime.vapi.enabled) {    
    int numu = vapi_num_unconnected (0);
    if (numu) {
      PRINTF ("\nWaiting for VAPI tests with ids:\n");
      vapi_num_unconnected (1);     
      PRINTF ("\n");
      while ((numu = vapi_num_unconnected (0))) {
        vapi_check ();
        PRINTF ("\rStill waiting for %i VAPI test(s) to connect.       ", numu);
        usleep (100);
      }
      PRINTF ("\n");
    }
    PRINTF ("All devices connected                         \n");
  }
  /* simulator is initialized */
  runtime.sim.init = 0;
}

/* Cleanup */
void sim_done (void)
{
  if (config.sim.profile) {
    fprintf(runtime.sim.fprof,"-%08llX FFFFFFFF\n", runtime.sim.cycles);
    fclose(runtime.sim.fprof);
  }
  
  if (config.sim.mprofile) fclose(runtime.sim.fmprof);
  if (config.sim.exe_log)   fclose(runtime.sim.fexe_log);
  if (runtime.vapi.enabled)  vapi_done ();
  done_memory_table ();
  exit(0);
}

/* Executes jobs in time queue. JPB - can't be static now it's in a 
 * separate file.
 */ 
#if !(DYNAMIC_EXECUTION)
/* static */ inline
#endif
void do_scheduler (void)
{
  struct sched_entry *tmp;

  /* Execute all jobs till now */
  do {  
    tmp = scheduler.job_queue;
    scheduler.job_queue = tmp->next;
    tmp->next = scheduler.free_job_queue;
    scheduler.free_job_queue = tmp;

    scheduler.job_queue->time += tmp->time;

#if DYNAMIC_EXECUTION
    /* This is done here and not after the loop has run because the job function
     * may raise an exception in which case set_sched_cycle would never be
     * called. */
    set_sched_cycle(scheduler.job_queue->time);
#endif
    TRACE_(sched)("Setting to-go cycles to %"PRIi32" at %lli\n",
                  scheduler.job_queue->time, runtime.sim.cycles);

    tmp->func (tmp->param);
  } while (scheduler.job_queue->time <= 0);
}

void recalc_do_stats(void)
{
  extern int do_stats;
  do_stats = config.cpu.superscalar || config.cpu.dependstats ||
             config.sim.history || config.sim.exe_log;
}

