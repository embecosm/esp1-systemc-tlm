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

//! Adds support for temporal decoupling to UartSyncSC::. Only the blocking
//! TLM callack, UartSyncSC::busReadWrite() is replaced, to support temporal
//! decoupling in the intitiator (Or1ksimDecoupSC::). The thread in this class
//! is not temporally decoupled, since it is not a TLM initiator.

class UartDecoupSC
  : public UartSyncSC
{
public:

  UartDecoupSC( sc_core::sc_module_name  name,
		unsigned long int        _clockRate,
		bool                     _isLittleEndian );


private:

  // Reimplemnted blocking transport function, which adds decoupled timing
  // delay. Will not be reimplemented further.

  virtual void  busReadWrite( tlm::tlm_generic_payload &payload,
			      sc_core::sc_time         &delay );

};	// UartDecoupSC()


#endif	// UART_DECOUP_SC__H


// EOF
