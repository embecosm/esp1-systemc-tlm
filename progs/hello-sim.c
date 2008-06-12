/* ----------------------------------------------------------------------------
 *
 *                  CONFIDENTIAL AND PROPRIETARY INFORMATION
 *                  ========================================
 *
 * Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.
 *
 * This file contains confidential and proprietary information of Embecosm and
 * is protected by copyright, trade secret and other regional, national and
 * international laws, and may be embodied in patents issued or pending.
 *
 * Receipt or possession of this file does not convey any rights to use,
 * reproduce, disclose its contents, or to manufacture, or sell anything it may
 * describe.
 *
 * Reproduction, disclosure or use without specific written authorization of
 * Embecosm is strictly forbidden.
 *
 * Reverse engineering is prohibited.
 *
 * ----------------------------------------------------------------------------
 *
 * Trivial test of the Or1ksim library
 *
 * $Id$
 *
 */


#include <stdlib.h>
#include <stdio.h>

#include "or1ksim.h"


main( int   argc,
      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: hello-sim <config_file> <image_file>\n" );
    exit( 1 );
  }

  if( or1ksim_init( argv[1], argv[2] ) == OR1KSIM_RC_OK ) {
    printf( "Successeful init\n" );
  }
  else {
    printf( "Init failed\n" );
    exit( 1 );
  }

  /* Run for 10000 cycles */

  printf( "Ran with result %d\n", or1ksim_run( 10000 ));
  
}	/* main() */
