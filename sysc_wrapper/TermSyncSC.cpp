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

// Implementation of xterm terminal emulator synchronous SystemC module

// $Id$


#include "TermSyncSC.h"


SC_HAS_PROCESS( TermSyncSC );

//! Custom constructor for the terminal module

//! Passes the name to the parent constructor.

//! Sets up threads listening to the rx port from the UART (TermSyncSC::rxThread)
//! and waiting for characters from the xterm (TermSyncSC::xtermThread).

//! Calculates the character delay appropriate for the baud rate. Fixed
//! behavior of 1 start bit, 8 data bits and and 1 stop bit.

//! Initializes the xterm in a separate process, setting up a file descriptor
//! that can read and write from/to it.

//! @param name      Name of this module - passed to the parent constructor
//! @param baudRate  The baud rate of the terminal connection to the UART

TermSyncSC::TermSyncSC( sc_core::sc_module_name  name,
			unsigned long int        baudRate ) :
  TermSC( name )  
{
  // Calculate the delay. No configurability here - 1 start, 8 data and 1 stop
  // = total 10 bits.

  charDelay = sc_core::sc_time( 10.0 / (double)baudRate, sc_core::SC_SEC );

}	/* TermSyncSC() */


//! Thread listening for characters from the xterm with synchronization

//! Wait to be notified via the SystemC event TermSC::ioEvent that there
//! is a character available.

//! Read the character, wait to model the delay on the wire, then send it out
//! to the UART

void
TermSyncSC::xtermThread()
{
  while( true ) {
    wait( *ioEvent );			// Wait for some I/O on the terminal

    int ch = xtermRead();		// Should not block

    wait( charDelay );			// Model baud rate delay
    tx.write( (unsigned char)ch );	// Send it
  }
}	// xtermThread()


// EOF
