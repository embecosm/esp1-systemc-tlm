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

// Definition of xterm terminal emulator SystemC object

// $Id$


#ifndef TERM_SC__H
#define TERM_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "or1ksim.h"


// A convenience struct for lists of FD -> instance mappings

struct Fd2Inst
{
  int           fd;
  class TermSC *inst;
  Fd2Inst      *next;

};	// Fd2Inst


// Module class for the Term. Talks to the outside world via two systemC
// fifos. Any data coming in has already been delayed appropriately via the
// UART. Any data we send out is similarly delayed.

class TermSC
: public sc_core::sc_module
{
 public:

  // Constructor and destructor

  TermSC( sc_core::sc_module_name  name,
	  unsigned long int        baudRate );
  ~TermSC();

  // Fifos for the UART to read/write to us

  sc_core::sc_fifo_in<unsigned char>   termRx;
  sc_core::sc_fifo_out<unsigned char>  termTx;


 private:

  // Threads for the Rx and Tx

  void  termRxThread();
  void  termTxThread();

  // Utility functions to control the xterm

  int   xtermInit();
  void  killTerm( const char *mess );
  void  launchTerm( char *slaveName );
  int   setupTerm();
  int   xtermRead();
  void  xtermWrite( char  ch );

  // Signal handling is tricky, since we have to use a static
  // function. Fortunately each instance has a 1:1 mapping to the FD used for
  // the xterm, so we can use that for lookup.

  static void  ioHandler( int        signum,
			  siginfo_t *si,
			  void      *p );

  static Fd2Inst    *instList;
  sc_core::sc_event *ioEvent;

  // xterm state

  int  ptMaster;		// FD of the master
  int  ptSlave;			// FD of the slave (in and out)
  int  xtermPid;		// Process ID of the child

  // Baud rate delay

  sc_core::sc_time  charDelay;

};	/* TermSC() */


#endif	// TERM_SC__H


// EOF
