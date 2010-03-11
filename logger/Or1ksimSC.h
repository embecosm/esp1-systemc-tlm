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

// Definition of the basic SystemC wrapper for Or1ksim

// $Id$


#ifndef OR1KSIM_SC__H
#define OR1KSIM_SC__H

#include <stdint.h>

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "or1ksim.h"


//! SystemC module class wrapping Or1ksim ISS

//! Provides a single thread (::run) which runs the underlying Or1ksim ISS.

class Or1ksimSC
  : public sc_core::sc_module
{
public:

  Or1ksimSC( sc_core::sc_module_name  name,
	     const char              *configFile,
	     const char              *imageFile );

  //! Initiator port for data accesses
  tlm_utils::simple_initiator_socket<Or1ksimSC>  dataBus;


protected:

  // Thread which will run the model. This will be reimplemented in later
  // derived classes to deal with timing.
  virtual void  run();

  // The common thread to make the transport calls. This will be reimplemented
  // in later derived classes to deal with timing.
  virtual void  doTrans( tlm::tlm_generic_payload &trans );


private:

  // I/O upcalls from Or1ksim, with a common synchronized transport
  // utility. These are not changed in later derived classes.

  static unsigned long int  staticReadUpcall( void              *instancePtr,
					      unsigned long int  addr,
					      unsigned long int  mask );

  static void               staticWriteUpcall( void              *instancePtr,
					       unsigned long int  addr,
					       unsigned long int  mask,
					       unsigned long int  wdata );

  uint32_t                  readUpcall( sc_dt::uint64  addr,
					uint32_t       mask );

  void                      writeUpcall( sc_dt::uint64  addr,
					 uint32_t       mask,
					 uint32_t       wdata );

};	/* Or1ksimSC() */


#endif	// OR1KSIM_SC__H

// EOF