/* toplevel.c -- Top level simulator source file
 * Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
 * Copyright (C) 2008 Jeremy Bennett, jeremy@jeremybennett.com
 *
 * This file is part of OpenRISC 1000 Architectural Simulator.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

/* Main program to drive the simulator. All support for simulator commands,
 * help and version output and SIGINT processing is now in
 * toplevel_support.c. Stdout redirection is specific to linux (Need to fix
 * this).
 */


/* System includes */

#include <unistd.h>

/* All the autoconf includes */

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/* Package includes */

#include "or1ksim.h"

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "debug_unit.h"
#include "vapi.h"
#include "gdbcomm.h"
#include "sched.h"
#include "profiler.h"
#include "mprofiler.h"


/* Support routines declared in toplevel_support.c */

extern void  ctrl_c( int  signum );
extern void  recalc_do_stats( void );
extern void  sim_init( void );
extern void  sim_done( void );
extern void  do_scheduler( void );


/* Main function */
int main( int   argc,
	  char *argv[] )
{
  srand(getpid());
  init_defconfig();
  reg_config_secs();
  if (parse_args(argc, argv)) {
    PRINTF("Usage: %s [options] <filename>\n", argv[0]);
    PRINTF("Options:\n");
    PRINTF(" -v                   version and copyright note\n");
    PRINTF(" -i                   enable interactive command prompt\n");
    PRINTF(" --nosrv              do not launch JTAG proxy server\n"); /* (CZ) */
    PRINTF(" --srv <n>            launch JTAG proxy server on port <n>; [random]\n"); /* (CZ) */
    PRINTF(" -f or --file         load script file [sim.cfg]\n");
    PRINTF(" -d <debug config>    Enable debug channels\n");
    PRINTF(" --enable-profile     enable profiling.\n"); 
    PRINTF(" --enable-mprofile    enable memory profiling.\n"); 
    PRINTF("\nor   : %s ", argv[0]);
    mp_help ();
    PRINTF("\nor   : %s ", argv[0]);
    prof_help ();
    exit(-1);
  }

  /* Read configuration file.  */
  if (!runtime.sim.script_file_specified)
    read_script_file ("sim.cfg");

  /* Overide parameters with command line ones */
  if (runtime.simcmd.profile) config.sim.profile = 1;
  if (runtime.simcmd.mprofile) config.sim.mprofile = 1;
  
  if (!runtime.sim.script_file_specified && config.sim.verbose) {
    fprintf( stderr,
	     "WARNING: No config file read, assuming default configuration.\n");
  }

  config.ext.class_ptr = NULL;		/* SystemC linkage disabled here. */
  config.ext.callback  = NULL;

  print_config();
  signal(SIGINT, ctrl_c);

  runtime.sim.hush = 1;
  recalc_do_stats();

  sim_init ();

  while(1) {
    long long time_start = runtime.sim.cycles;
    if (config.debug.enabled) {
      du_clock(); // reset watchpoints
      while (runtime.cpu.stalled) {
        if(config.debug.gdb_enabled) {
          BlockJTAG();
          HandleServerSocket(false);
        } else {
          fprintf( stderr,
		   "WARNING: CPU stalled and gdb connection not enabled.\n");
          /* Dump the user into interactive mode.  From there he can decide what
           * to do. */
          handle_sim_command();
          sim_done();
        }
        if(runtime.sim.iprompt)
          handle_sim_command();
      }
    }

    /* Each cycle has counter of mem_cycles; this value is joined with cycles
       at the end of the cycle; no sim originated memory accesses should be
       performed inbetween. */
    runtime.sim.mem_cycles = 0;
    if (!config.pm.enabled ||
        !(cpu_state.sprs[SPR_PMR] & (SPR_PMR_DME | SPR_PMR_SME)))
      if (cpu_clock ())
        /* A breakpoint has been hit, drop to interactive mode */
        handle_sim_command();

    if (config.vapi.enabled && runtime.vapi.enabled) vapi_check();
    if (config.debug.gdb_enabled) HandleServerSocket(false); /* block & check_stdin = false */
    if(config.debug.enabled)
      if (cpu_state.sprs[SPR_DMR1] & SPR_DMR1_ST) set_stall_state (1);

    runtime.sim.cycles += runtime.sim.mem_cycles;
    scheduler.job_queue->time -= runtime.sim.cycles - time_start;
    if (scheduler.job_queue->time <= 0) do_scheduler ();
  }
  sim_done();
}
