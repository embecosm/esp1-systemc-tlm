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

// Implementation of main SystemC wrapper for the OSCI SystemC wrapper project

// $Id$


#include "Or1ksimExtSC.h"


//! Custom constructor for the Or1ksimExtSC extended SystemC module

//! Just calls the parent contstructor.

//! @param name        SystemC module name
//! @param configFile  Config file for the underlying ISS
//! @param imageFile   Binary image to run on the ISS


Or1ksimExtSC::Or1ksimExtSC ( sc_core::sc_module_name  name,
			     const char              *configFile,
			     const char              *imageFile ) :
  Or1ksimSC( name, configFile, imageFile )
{
 }	// Or1ksimExtSC()


//! Identify the endianism of the underlying Or1ksim ISS

//! Public utility to allow other modules to identify the model endianism

//! @return  True if the Or1ksim ISS is little endian

bool
Or1ksimExtSC::isLittleEndian()
{
  return (1 == or1ksim_is_le());

}	// or1ksimIsLe()


//! Extended TLM transport to the target

//! Calls the blocking transport routine for the initiator socket (@see
//! ::dataBus).

//! This version adds a zero time delay call to wait() so that the thread will
//! yield in an untimed model.

//! @param trans  The transaction payload

void
Or1ksimExtSC::doTrans( tlm::tlm_generic_payload &trans )
{
  sc_core::sc_time  dummyDelay = sc_core::SC_ZERO_TIME;

  // Call the transport and wait for no time, which allows the thread to yield
  // and others to get a look in!

  dataBus->b_transport( trans, dummyDelay );
  wait( sc_core::SC_ZERO_TIME );

}	// doTrans()


// EOF
