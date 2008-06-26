/* libtoplevel.c -- Top level simulator library source file
 *
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
 *
 * $Id*
 *
 */

/* Top level library routines to drive the simulator. All support for
 * simulator commands, help and version output and SIGINT processing is now in
 * toplevel_support.c. Stdout redirection is specific to linux (Need to fix
 * this).
 *
 * Library API provides:
 *
 * or1ksim_init()   Initializes the simulator, with optional config file
 * or1ksim_run()    Run for a given time
 * or1ksim_read()   Read from the simulator's memory
 * or1ksim_write()  Write to the simulator's memory
 *
 * Full details in the commenting with each routine.
 */


/* System includes */

#include <unistd.h>

/* All the autoconf includes */

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/* Or1ksim package includes */

#include "or1ksim.h"		/* The library header */

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


/* Initialize the simulator. 
 *
 * Allows specification of an (optional) config file and an image file. Builds
 * up dummy argc/argv to pass to the existing argument parser.

 * Returns 0 on success and an error code on failure

 */

void (*jpb_callback)( void       *class_ptr,
		      const char *mess );
void *jpb_class_ptr;

int  or1ksim_init( const char         *config_file,
		   const char         *image_file,
		   void               *class_ptr,
		   unsigned long int (*upr)( void              *class_ptr,
					     unsigned long int  addr,
					     unsigned long int  mask ),
		   void              (*upw)( void              *class_ptr,
					     unsigned long int  addr,
					     unsigned long int  mask,
					     unsigned long int  wdata ) )
{
  int   dummy_argc = 4;
  char *dummy_argv[4];

  /* Dummy argv array */

  dummy_argv[0] = "libsim";
  dummy_argv[1] = "-f";
  dummy_argv[2] = (char *)((NULL != config_file) ? config_file : "sim.cfg");
  dummy_argv[3] = (char *)image_file;

  /* Initialization copied from existing main() */

  srand(getpid());
  init_defconfig();
  reg_config_secs();

  if( parse_args( dummy_argc, dummy_argv )) {
    return  OR1KSIM_RC_BADINIT;
  }

  config.sim.profile   = 0;		/* No profiling */
  config.sim.mprofile  = 0;

  config.ext.class_ptr = class_ptr;	/* SystemC linkage */
  config.ext.read_up   = upr;
  config.ext.write_up  = upw;

  print_config();			/* Will go eventually */
  signal( SIGINT, ctrl_c );		/* Not sure we want this really */

  runtime.sim.hush = 1;			/* Not sure if this is needed */
  recalc_do_stats();

  sim_init ();

  runtime.sim.ext_int = 0;		/* No interrupts pending */

  return  OR1KSIM_RC_OK;

}	/* or1ksim_init() */


/* Run the simulator. The argument is a time in seconds, which is converted to
 * a number of cycles, if positive. A negative value means "run for ever".

 * The semantics are that the duration for which the run may occur may be
 * changed mid-run by a call to or1ksim_reset_duration(). This is to allow for
 * the upcalls to generic components adding time, and reducing the time
 * permitted for ISS execution before synchronization of the parent SystemC
 * wrapper. 

 * This is over-ridden if the call was for a negative duration, which means
 * run forever!

 * Uses a simplified version of the old main program loop. Returns success if
 * the requested number of cycles were run and an error code otherwise.
 */

int  or1ksim_run( double  duration )
{
  or1ksim_reset_duration( duration );

  /* Loop until we have done enough cycles (or forever if we had a negative
   *  duration.
   */

  while( duration < 0.0 || (runtime.sim.cycles < runtime.sim.end_cycles) ) {

    long long int  time_start = runtime.sim.cycles;
    int            i;					/* Interrupt # */

    /* Each cycle has counter of mem_cycles; this value is joined with cycles
     * at the end of the cycle; no sim originated memory accesses should be
     * performed inbetween.
     */

    runtime.sim.mem_cycles = 0;
    
    if( cpu_clock () ) {
      return OR1KSIM_RC_BRKPT;		/* Hit a breakpoint */
    }

    runtime.sim.cycles += runtime.sim.mem_cycles;

    /* Taken any external interrupts. Outer test is for the common case for
       efficiency. */

    if( 0 != runtime.sim.ext_int ) {
      for( i = 0 ; i < sizeof (runtime.sim.ext_int) ; i++ ) {
	if( 0x1 == ((runtime.sim.ext_int >> i) & 0x1) ) {
	  report_interrupt( i );
	  runtime.sim.ext_int &= ~(1 << i);	/* Clear int */
	}
      }
    }

    /* Update the scheduler queue */

    scheduler.job_queue->time -= (runtime.sim.cycles - time_start);

    if (scheduler.job_queue->time <= 0) {
      do_scheduler ();
    }
  }

}	/* or1ksim_run() */


/* Reset the run-time simulation end point */

void  or1ksim_reset_duration( double duration )
{
  runtime.sim.end_cycles = 
    runtime.sim.cycles +
    (long long int)(duration * 1.0e12 / (double)config.sim.clkcycle_ps);

}	/* or1ksim_reset_duration() */


/* Internal utility to return the time (in seconds) executed so far. Note that
   this is a re-entrant routine. */

static double  internal_or1ksim_time()
{
  return (double)runtime.sim.cycles * (double)config.sim.clkcycle_ps / 1.0e12;

}	// or1ksim_cycle_count()


/* Mark a time point in the simulation */

void  or1ksim_set_time_point()
{
  runtime.sim.time_point = internal_or1ksim_time();

}	/* or1ksim_set_time_point() */


/* Return the time since the time point was set */

double  or1ksim_get_time_period()
{
  return  internal_or1ksim_time() - runtime.sim.time_point;

}	/* or1ksim_get_time_period() */


/* Simple utility to return the endianism of the model. Return 1 if the model
   is little endian, 0 otherwise. Note that this is a re-entrant routine. */

int  or1ksim_is_le()
{
#ifdef WORDS_BIGENDIAN
  return 0;
#else
  return 1;
#endif

}	// or1ksim_is_le()


/* Simple utility to return the clock rate */

unsigned long int  or1ksim_clock_rate()
{
  return (unsigned long int)(1000000000000ULL /
			     (unsigned long long int)(config.sim.clkcycle_ps));

}	// or1ksim_clock_rate()


/* Take an interrupt */

void or1ksim_interrupt( int  i )
{
  runtime.sim.ext_int |= 1 << i;		// Better not be > 31!

}	/* or1ksim_interrupt() */
