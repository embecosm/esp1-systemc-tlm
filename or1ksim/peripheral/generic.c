/* generic.c -- Generic external peripheral
 *
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

/* This is functional simulation of any external peripheral. It's job is to
 * trap accesses in a specific range, so that the simulator can drive an
 * external device.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "or1ksim.h"
#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "pic.h"
#include "vapi.h"
#include "sched.h"
#include "channel.h"
#include "debug.h"

#include "generic.h"


DEFAULT_DEBUG_CHANNEL( generic );


/* Generic callback routine. Note the address here is aboslute, not relative
   to the device. */

static unsigned long int  ext_callback( enum or1ksim_cb    code,
					unsigned long int  addr,
					unsigned long int  wdata )
{
  return config.ext.callback( config.ext.class_ptr, code, addr, wdata );

}	/* ext_callback() */
		       

/* I/O routines. Note that address is relative to start of address space. */

static uint8_t  generic_read_byte( oraddr_t  addr,
				   void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Byte read from disabled generic device\n" );
    return 0;
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Byte read out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
    return 0;
  }
  else {
    return (uint8_t)ext_callback( OR1KSIM_CB_BYTE_R,
				  (unsigned long int)(addr + dev->baseaddr),
				  0UL );
  }
}	/* generic_read_byte() */


static void  generic_write_byte( oraddr_t  addr,
				 uint8_t   value,
				 void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Byte write to disabled generic device\n" );
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Byte write out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
  }
  else {
    (void)ext_callback( OR1KSIM_CB_BYTE_W,
			(unsigned long int)(addr + dev->baseaddr),
			(unsigned long int)value );
  }
}	/* generic_write_byte() */


static uint16_t  generic_read_hw( oraddr_t  addr,
				  void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Half word read from disabled generic device\n" );
    return 0;
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Half word read out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
    return 0;
  }
  else {
    return (uint16_t)ext_callback( OR1KSIM_CB_HW_R,
				   (unsigned long int)(addr + dev->baseaddr),
				   0UL );
  }
}	/* generic_read_hw() */


static void  generic_write_hw( oraddr_t  addr,
			       uint16_t   value,
			       void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Half word write to disabled generic device\n" );
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Half word write out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
  }
  else{
    (void)ext_callback( OR1KSIM_CB_HW_W,
			(unsigned long int)(addr + dev->baseaddr),
			(unsigned long int)value );
  }
}	/* generic_write_hw() */


static uint32_t  generic_read_word( oraddr_t  addr,
				    void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Full word read from disabled generic device\n" );
    return 0;
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Full word read out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
    return 0;
  }
  else {
    return (uint32_t)ext_callback( OR1KSIM_CB_WORD_R,
				   (unsigned long int)(addr + dev->baseaddr),
				   0UL );
  }
}	/* generic_read_word() */


static void  generic_write_word( oraddr_t  addr,
				 uint32_t   value,
				 void     *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  if( !config.ext.class_ptr ) {
    fprintf( stderr, "Full word write to disabled generic device\n" );
  }
  else if( addr >= dev->size ) {
    TRACE( "Generic device \"%s\": Full word write out of range (addr %"
	   PRIxADDR ")\n", dev->name, addr );
  }
  else{
    (void)ext_callback( OR1KSIM_CB_WORD_W,
			(unsigned long int)(addr + dev->baseaddr),
			(unsigned long int)value );
  }
}	/* generic_write_word() */


/* Reset is a null operation */

static void  generic_reset( void *dat )
{
  return;

}	/* generic_reset() */


/* Status report can only advise of configuration. */

static void  generic_status( void *dat )
{
  struct dev_generic *dev = (struct dev_generic *)dat;

  PRINTF( "\nGeneric device \"%s\" at 0x%" PRIxADDR ":\n", dev->name,
	  dev->baseaddr );
  PRINTF( "  Size 0x%" PRIx32 "\n", dev->size );

  if( dev->byte_enabled ) {
    PRINTF( "  Byte R/W enabled\n" );
  }

  if( dev->hw_enabled ) {
    PRINTF( "  Half word R/W enabled\n" );
  }

  if( dev->word_enabled ) {
    PRINTF( "  Full word R/W enabled\n" );
  }

  PRINTF( "\n" );

}	/* generic_status() */


/* Functions to set configuration */

static void  generic_enabled( union param_val  val,
			      void            *dat )
{
  ((struct dev_generic *)dat)->enabled = val.int_val;

}	/* generic_enabled() */


static void  generic_byte_enabled( union param_val  val,
				   void            *dat )
{
  ((struct dev_generic *)dat)->byte_enabled = val.int_val;

}	/* generic_byte_enabled() */


static void  generic_hw_enabled( union param_val  val,
				 void            *dat )
{
  ((struct dev_generic *)dat)->hw_enabled = val.int_val;

}	/* generic_hw_enabled() */


static void  generic_word_enabled( union param_val  val,
				   void            *dat )
{
  ((struct dev_generic *)dat)->word_enabled = val.int_val;

}	/* generic_word_enabled() */


static void  generic_name( union param_val  val,
			   void            *dat )
{
  ((struct dev_generic *)dat)->name = strdup( val.str_val );

  if( !((struct dev_generic *)dat)->name ) {
    fprintf(stderr, "Peripheral 16450: name \"%s\": Run out of memory\n",
	    val.str_val );
    exit(-1);
  }
}	/* generic_name() */


static void  generic_baseaddr( union param_val  val,
			       void            *dat )
{
  ((struct dev_generic *)dat)->baseaddr = val.addr_val;

}	/* generic_baseaddr() */


static void  generic_size( union param_val  val,
			      void            *dat )
{
  ((struct dev_generic *)dat)->size = val.int_val;

}	/* generic_size() */


/* Start of new generic section */

static void *generic_sec_start()
{
  struct dev_generic *new =
    (struct dev_generic *)malloc( sizeof( struct dev_generic ));

  if( 0 == new) {
    fprintf( stderr, "Generic peripheral: Run out of memory\n" );
    exit( -1 );
  }

  /* Default names */

  new->enabled      = 1;
  new->byte_enabled = 1;
  new->hw_enabled   = 1;
  new->word_enabled = 1;
  new->name         = "anonymous external peripheral";
  new->baseaddr     = 0;
  new->size         = 0;

  return  new;

}	/* generic_sec_start() */


/* End of new generic section */

static void  generic_sec_end( void *dat)
{
  struct dev_generic *generic = (struct dev_generic *)dat;
  struct mem_ops      ops;

  /* Give up if not enabled, or if size is zero, or if no access size is
     enabled. */

  if( !generic->enabled ) {
    free( dat );
    return;
  }

  if( 0 == generic->size ) {
    fprintf( stderr, "Generic peripheral \"%s\" has size 0: ignoring",
	     generic->name );
    free( dat );
    return;
  }

  if( !generic->byte_enabled &&
      !generic->hw_enabled   &&
      !generic->word_enabled ) {
    fprintf( stderr, "Generic peripheral \"%s\" has no access: ignoring",
	     generic->name );
    free( dat );
    return;
  }

  /* Zero all the ops, then set the ones we care about. Read/write delays will
   * come from the peripheral if desired.
   */

  memset( &ops, 0, sizeof( struct mem_ops ));

  if( generic->byte_enabled ) {
    ops.readfunc8 = generic_read_byte;
    ops.writefunc8 = generic_write_byte;
    ops.read_dat8 = dat;
    ops.write_dat8 = dat;
  }

  if( generic->hw_enabled ) {
    ops.readfunc16 = generic_read_hw;
    ops.writefunc16 = generic_write_hw;
    ops.read_dat16 = dat;
    ops.write_dat16 = dat;
  }

  if( generic->word_enabled ) {
    ops.readfunc32 = generic_read_word;
    ops.writefunc32 = generic_write_word;
    ops.read_dat32 = dat;
    ops.write_dat32 = dat;
  }

  /* Register everything */

  reg_mem_area( generic->baseaddr, generic->size, 0, &ops );

  reg_sim_reset( generic_reset,  dat );
  reg_sim_stat(  generic_status, dat );

}	/* generic_sec_end() */


/* Register a generic section. */

void reg_generic_sec(void)
{
  struct config_section *sec = reg_config_sec( "generic",
					       generic_sec_start,
                                               generic_sec_end );

  reg_config_param( sec, "enabled",      paramt_int,  generic_enabled      );
  reg_config_param( sec, "byte_enabled", paramt_int,  generic_byte_enabled );
  reg_config_param( sec, "hw_enabled",   paramt_int,  generic_hw_enabled   );
  reg_config_param( sec, "word_enabled", paramt_int,  generic_word_enabled );
  reg_config_param( sec, "name",         paramt_str,  generic_name         );
  reg_config_param( sec, "baseaddr",     paramt_addr, generic_baseaddr     );
  reg_config_param( sec, "size",         paramt_int,  generic_size         );

}	/* reg_generic_sec */
