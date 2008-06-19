/* or1ksim.h -- Simulator library header file
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
 */

/* Header file definining the interface to the Or1ksim library. */

/* $Id$ */


#ifndef OR1KSIM__H
#define OR1KSIM__H


/* The bus width */

/* #define BUSWIDTH  32 */

/* The return codes */

enum  or1ksim_rc {
  OR1KSIM_RC_OK,		/* No error */

  OR1KSIM_RC_BADINIT,		/* Couldn't initialize */
  OR1KSIM_RC_BRKPT		/* Hit a breakpoint */
};

/* The interface methods */

#ifdef __cplusplus
extern "C" {
#endif

int  or1ksim_init( const char         *config_file,
		   const char         *image_file,
		   void               *class_ptr,
		   unsigned long int (*upr)( void              *class_ptr,
					     unsigned long int  addr,
					     unsigned long int  mask),
		   void              (*upw)( void              *class_ptr,
					     unsigned long int  addr,
					     unsigned long int  mask,
					     unsigned long int  wdata ) );

int  or1ksim_run( double  duration );

int  or1ksim_is_le();

double  or1ksim_time();

#ifdef __cplusplus
}
#endif


#endif	/* OR1KSIM__H */