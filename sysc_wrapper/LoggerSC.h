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

// Definition of the simple logger object

// $Id$


#ifndef LOGGER_SC__H
#define LOGGER_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"


// Module class for the logger. It has a simple target socket for the CPU to
// read/write registers

class LoggerSC
: public sc_core::sc_module
{
 public:

  // Constructor and callback

  LoggerSC( sc_core::sc_module_name  name );

  tlm_utils::simple_target_socket<LoggerSC>  loggerPort;


 private:

  // The blocking transport routine for the port

  void  loggerReadWrite( tlm::tlm_generic_payload &payload,
			 sc_core::sc_time         &delayTime );


};	// LoggerSC()


#endif	// LOGGER_SC__H


// EOF
