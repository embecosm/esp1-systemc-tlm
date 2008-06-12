// ----------------------------------------------------------------------------

//                  CONFIDENTIAL AND PROPRIETARY INFORMATION
//                  ========================================

// Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.

// This file contains confidential and proprietary information of Embecosm and
// is protected by copyright, trade secret and other regional, national and
// international laws, and may be embodied in patents issued or pending.

// Receipt or possession of this file does not convey any rights to use,
// reproduce, disclose its contents, or to manufacture, or sell anything it may
// describe.

// Reproduction, disclosure or use without specific written authorization of
// Embecosm is strictly forbidden.

// Reverse engineering is prohibited.

// ----------------------------------------------------------------------------

// Implementation of on-blocking top level SystemC method for the OSCI SystemC
// wrapper project

// $Id$

#include "tlm.h"
#include "Or1ksimSC.h"
#include "DataReporterSC.h"


int  sc_main( int   argc,
	      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: hello-sim <config_file> <image_file>\n" );
    exit( 1 );
  }

  Or1ksimSC       top( "top", argv[1], argv[2] );
  DataReporterSC  log( "log" );

  // Connect up the ports

  top.dataIni( log.dataTar );

  sc_core::sc_start();

  return 0;

}	// sc_main()
