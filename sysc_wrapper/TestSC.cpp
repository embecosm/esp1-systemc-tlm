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

// Top level for test of Or1ksim with logger

// $Id$

#include "tlm.h"
#include "Or1ksimSC.h"
#include "LoggerSC.h"


int  sc_main( int   argc,
	      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: TestSC <config_file> <image_file>\n" );
    exit( 1 );
  }

  Or1ksimSC  iss( "or1ksim", argv[1], argv[2] );
  LoggerSC   logger( "logger" );

  // Connect up the TLM ports

  iss.dataBus( logger.loggerSocket );

  // Run it forever

  sc_core::sc_start();

  return 0;

}	// sc_main()
