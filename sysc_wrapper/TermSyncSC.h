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

#include "TermSC.h"


//! SystemC module class for the Terminal with synchronization

//! Talks to the outside world via two systemC FIFOs. Any data coming in has
//! already been delayed (to represent the baud rate wire delay) via the
//! UART. Any data we send out is similarly delayed.

class TermSyncSC
  : public TermSC
{
public:

  // Constructor

  TermSyncSC( sc_core::sc_module_name  name,
	      unsigned long int        baudRate );


private:

  // Updated thread with timing to model the baud rate delay. Will not be
  // modified further in derived classes.

  void  xtermThread();

  // Baud rate delay

  sc_core::sc_time  charDelay;	//!< Baud rate delay to send a character

};	/* TermSyncSC() */


#endif	// TERM_SYNC_SC__H


// EOF
