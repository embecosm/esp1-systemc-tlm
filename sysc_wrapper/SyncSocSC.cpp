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
#include "Or1ksimSyncSC.h"
#include "UartSyncSC.h"
#include "TermSyncSC.h"

#define BAUD_RATE  9600UL		// Baud rate of the terminal

int  sc_main( int   argc,
	      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: hello-sim <config_file> <image_file>\n" );
    exit( 1 );
  }

  Or1ksimSyncSC  iss( "or1ksim", argv[1], argv[2] );
  UartSyncSC     uart( "uart", iss.getClockRate(), iss.isLittleEndian() );
  TermSyncSC     term( "terminal", BAUD_RATE );

  // Connect up the TLM ports

  iss.dataBus( uart.bus );

  // Connect up the UART and terminal via a 1-byte fifo.

  sc_core::sc_fifo<unsigned char>  u2t(1);
  sc_core::sc_fifo<unsigned char>  t2u(1);

  uart.rx( t2u );
  uart.tx( u2t );
  term.rx( u2t );
  term.tx( t2u );

  // Signal for the interrupt, which is just left dangling for now

  sc_core::sc_signal<bool>  intWire;

  uart.intr( intWire );

  // Run it forever

  sc_core::sc_start();

  return 0;

}	// sc_main()
