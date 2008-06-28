// ----------------------------------------------------------------------------

// Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

// Copyright (C) 2008  Embecosm Limited <info@embecosm.com>

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// ----------------------------------------------------------------------------

// Implementation of 16450 UART SystemC module with temporal decoupling

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
