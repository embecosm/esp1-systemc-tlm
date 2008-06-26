// ----------------------------------------------------------------------------

// Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

// Copyright (C) 2008  Embecosm Limited

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
// with SystemC time synchronization

// $Id$


#ifndef OR1KSIM_SYNC_SC__H
#define OR1KSIM_SYNC_SC__H

#include "Or1ksimExtSC.h"


//! SystemC module class wrapping Or1ksim ISS with synchronized timing

//! Derived from the earlier Or1ksimExtSC class. Reimplements
//! Or1ksimExtSC::doTrans() to provide synchronized timing.

class Or1ksimSyncSC
  : public Or1ksimExtSC
{
public:

  Or1ksimSyncSC( sc_core::sc_module_name  name,
		 const char              *configFile,
		 const char              *imageFile );

  // Public utility to return the clock rate
  unsigned long int  getClockRate();


protected:

  // The common thread to make the transport calls. This has synchronized
  // timing. It will be further reimplemented in later calls to add termporal
  // decoupling.
  virtual void  doTrans( tlm::tlm_generic_payload &trans );

};	/* Or1ksimSyncSC() */


#endif	// OR1KSIM_SYNC_SC__H

// EOF
