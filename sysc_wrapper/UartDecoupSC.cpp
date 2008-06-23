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

// Implementation of 16450 UART Synchronous SystemC module.

// $Id$


#include "UartDecoupSC.h"


//! Custom constructor for the UART module

//! Calls the parent constructor

//! @param name             The SystemC module name, passed to the parent
//!                         constructor
//! @param _clockRate       The external clock rate, passed to the parent
//!                         consturctor
//! @param _isLittleEndian  The model endianism

UartDecoupSC::UartDecoupSC( sc_core::sc_module_name  name,
			unsigned long int        _clockRate,
			bool                     _isLittleEndian ) :
  UartSyncSC( name, _clockRate, _isLittleEndian )
{

}	/* UartDecoupSC() */


//! TLM2.0 blocking transport routine for the UART bus socket with decoupled
//! timing.

//! Reuses the base class function, UartSC::busReadWrite(), but updates the
//! delay. Does not synchronize - that is for the initiator to decide.

//! @param payload  The transaction payload
//! @param delay    How far the initiator is beyond baseline SystemC time.

void
UartDecoupSC::busReadWrite( tlm::tlm_generic_payload &payload,
			  sc_core::sc_time         &delay )
{
  UartSC::busReadWrite( payload, delay );	// base method

  // Delay as appropriate.

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:
    delay += sc_core::sc_time( UART_READ_NS, sc_core::SC_NS );
    break;

  case tlm::TLM_WRITE_COMMAND:
    delay += sc_core::sc_time( UART_WRITE_NS, sc_core::SC_NS );
    break;
  }

}	// busReadWrite()


// EOF
