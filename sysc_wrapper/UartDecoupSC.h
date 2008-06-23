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

// Definition of 16450 UART Temporally Decoupled SystemC module.

// $Id$


#ifndef UART_DECOUP_SC__H
#define UART_DECOUP_SC__H

#include "UartSyncSC.h"

//! SystemC module class for a 16450 UART with temporal decoupling

//! Provides a TLM 2.0 simple target convenience socket for access to the UART
//! regsters, unsigned char SystemC FIFO ports (to a 1 byte FIFO) for the Rx
//! and Tx pins and a bool SystemC signal for the interrupt pin.

//! Two threads are provided, one waiting for transmit requests from the bus,
//! the other waiting for data on the Rx pin.

class UartDecoupSC
: public UartSyncSC
{
 public:

  // Constructor

  UartDecoupSC( sc_core::sc_module_name  name,
		unsigned long int        _clockRate,
		bool                     _isLittleEndian );


 protected:

  // Updated blocking transport function, which adds decoupled timing
  // delay.

  virtual void  busReadWrite( tlm::tlm_generic_payload &payload,
			      sc_core::sc_time         &delay );

};	// UartDecoupSC()


#endif	// UART_DECOUP_SC__H


// EOF
