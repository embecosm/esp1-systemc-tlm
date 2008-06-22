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

// Definition of xterm terminal emulator synchronous SystemC module

// $Id$


#ifndef TERM_SYNC_SC__H
#define TERM_SYNC_SC__H

#include <signal.h>

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "or1ksim.h"


//! A convenience struct for lists of file descriptor -> instance mappings

struct Fd2Inst
{
  int           fd;		//!< The file descriptor
  class TermSyncSC *inst;		//!< The instance (of class TermSyncSC)
  Fd2Inst      *next;		//!< Next element in the list

};	// Fd2Inst


//! SystemC module class for the Terminal.

//! Talks to the outside world via two systemC FIFOs. Any data coming in has
//! already been delayed (to represent the baud rate wire delay) via the
//! UART. Any data we send out is similarly delayed.

//! The terminal is implemented as a separate process running an xterm. Two
//! threads are required, one to listen for characters typed to the xterm, the
//! other to listen for characters sent from the UART.

class TermSyncSC
: public sc_core::sc_module
{
 public:

  // Constructor and destructor

  TermSyncSC( sc_core::sc_module_name  name,
	  unsigned long int        baudRate );
  ~TermSyncSC();

  // Fifos for the UART to read/write to us

  sc_core::sc_fifo_in<unsigned char>   rx;		//!< FIFO for bytes in
  sc_core::sc_fifo_out<unsigned char>  tx;		//!< FIFO for bytes out


 private:

  // Threads for the Rx and Tx

  void  rxThread();
  void  xtermThread();

  // Utility functions to control the xterm

  int            xtermInit();
  void           xtermKill( const char *mess );
  void           xtermLaunch( char *slaveName );
  int            xtermSetup();

  unsigned char  xtermRead();
  void           xtermWrite( unsigned char  ch );

  // Signal handling is tricky, since we have to use a static
  // function. Fortunately each instance has a 1:1 mapping to the FD used for
  // the xterm, so we can use that for lookup.

  static void  ioHandler( int        signum,
			  siginfo_t *si,
			  void      *p );

  //! The list of mappings of file descriptors to TermSyncSC instances
  static Fd2Inst    *instList;

  //! Pointer to the SystemC event raised when input is available

  //! @note This must be a pointer. If an event were declared here it would be
  //!       available at elaboration time and cause a crash. It is allocated
  //!       when the xterm is created.
  sc_core::sc_event *ioEvent;

  // xterm state

  int  ptMaster;	//!< Master file descriptor
  int  ptSlave;		//!< Slave file descriptor (for talking to the xterm)
  int  xtermPid;	//!< Process ID of the child running the xterm

  // Baud rate delay

  sc_core::sc_time  charDelay;	//!< Baud rate delay to send a character

};	/* TermSyncSC() */


#endif	// TERM_SYNC_SC__H


// EOF
