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


//! SystemC module class providing a simple TLM 2.0 logger

//! Provides a TLM 2.0 simple target convenience socket, and records the
//! command, address, data, mask and delay in any packets sent to that socket.

//! Assumes all transactions are 32 bits long - this is intended for use with
//! the Or1ksimSC class.

class LoggerSC
: public sc_core::sc_module
{
 public:

  // Constructor and callback

  LoggerSC( sc_core::sc_module_name  name );

  tlm_utils::simple_target_socket<LoggerSC>  loggerSocket;


 private:

  // The blocking transport routine for the port

  void  loggerReadWrite( tlm::tlm_generic_payload &payload,
			 sc_core::sc_time         &delay );


};	// LoggerSC()


#endif	// LOGGER_SC__H


// EOF
