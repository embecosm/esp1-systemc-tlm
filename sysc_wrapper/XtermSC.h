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


#ifndef XTERM_SC__H
#define XTERM_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "or1ksim.h"

//! Module class for the Xterm.

class XtermSC
: public sc_core::sc_module
{
 public:

  // Constructor and destructor

  XtermSC( sc_core::sc_module_name  name );
  ~XtermSC();

  // Blocking transport method - called whenever an initiator wants to read or
  // write from/to us.

  void  doReadWrite( tlm::tlm_generic_payload &payload,
		     sc_core::sc_time         &delayTime );

  // Target port for devices to read or write to us
  
  tlm_utils::simple_target_socket<XtermSC>  xtermPort;	// Target port


 private:

  // Split out read and write for clarity

  void  doRead( tlm::tlm_generic_payload &payload,
		sc_core::sc_time         &delayTime );

  void  doWrite( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime );

  // Utility functions to control the xterm

  int   xtermInit();
  void  launchXterm( char *slaveName );
  int  xtermRead();
  int  xtermWrite( char  ch );

  // xterm state

  int  ptMaster;		// FD of the master
  int  ptSlave;			// FD of the slave (in and out)
  int  xtermPid;		// Process ID of the child

};	/* XtermSC() */


#endif	// XTERM_SC__H


// EOF
