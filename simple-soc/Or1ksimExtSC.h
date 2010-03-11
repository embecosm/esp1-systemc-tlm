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

// Definition of main SystemC wrapper for the OSCI SystemC wrapper project
// extened to yield on upcalls

// $Id$


#ifndef OR1KSIM_EXT_SC__H
#define OR1KSIM_EXT_SC__H

#include "Or1ksimSC.h"


//! Extended SystemC module class wrapping Or1ksim ISS

//! Derived from Or1ksimSC:: to reimplement Or1ksimSC::doTrans(). Everything
//! else comes from the parent module.

class Or1ksimExtSC
  : public Or1ksimSC
{
public:

  Or1ksimExtSC( sc_core::sc_module_name  name,
		const char              *configFile,
		const char              *imageFile );


protected:

  // Reimplmented version of the common thread to make the transport
  // calls. This will be further reimplemented in later derived classes to
  // deal with timing.
  virtual void  doTrans( tlm::tlm_generic_payload &trans );

};	/* Or1ksimExtSC() */


#endif	// OR1KSIM_EXT_SC__H

// EOF
