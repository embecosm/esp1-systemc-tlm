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
// with SystemC temporal decoupling

// $Id$


#ifndef OR1KSIM_DECOUP_SC__H
#define OR1KSIM_DECOUP_SC__H

#include "Or1ksimSyncSC.h"
#include "tlm_utils/tlm_quantumkeeper.h"


//! SystemC module class wrapping Or1ksim ISS with temporal decoupling

//! Derived from the earlier Or1ksimSyncSC class. Reimplements the
//! Or1ksimSyncSC::run() thread to support temporal decoupling. Reimplements
//! the Or1ksimSynSC::doTrans() method to support temporal decoupling.

class Or1ksimDecoupSC
  : public Or1ksimSyncSC
{
public:

  Or1ksimDecoupSC( sc_core::sc_module_name  name,
		   const char              *configFile,
		   const char              *imageFile );


protected:

  // The common thread to make the transport calls. This has temporal
  // decoupling. Will be reimplemented in later calls
  virtual void  doTrans( tlm::tlm_generic_payload &trans );


private:

  // Thread which will run the model, with temporal decoupling. No more
  // reimplementation.
  void  run();

  //! TLM 2.0 Quantum keeper for the ISS model thread. Won't be used in any
  //! derived classes.
  tlm_utils::tlm_quantumkeeper  issQk;

};	/* Or1ksimDecoupSC() */


#endif	// OR1KSIM_DECOUP_SC__H

// EOF