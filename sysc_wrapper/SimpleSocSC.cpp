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

// Top level simple SoC.

// $Id$

#include "tlm.h"
#include "Or1ksimExtSC.h"
#include "UartSC.h"
#include "TermSC.h"

int  sc_main( int   argc,
	      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: hello-sim <config_file> <image_file>\n" );
    exit( 1 );
  }

  Or1ksimExtSC  iss( "or1ksim", argv[1], argv[2] );
  UartSC        uart( "uart", iss.isLittleEndian() );
  TermSC        term( "terminal" );

  // Connect up the TLM ports

  iss.dataBus( uart.bus );

  // Connect up the UART and terminal Tx and Rx

  uart.tx( term.rx );
  term.tx( uart.rx );

  // Run it forever

  sc_core::sc_start();

  return 0;

}	// sc_main()
