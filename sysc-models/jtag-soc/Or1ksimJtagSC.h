// Wrapper for Or1ksim with interrupts and JTAG module header

// Copyright (C) 2008, 2010 Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

// This file is part of the example programs for "Building a Loosely Timed SoC
// Model with OSCI TLM 2.0"

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// This code is commented throughout for use with Doxygen.
// ----------------------------------------------------------------------------

#ifndef OR1KSIM_JTAG_SC__H
#define OR1KSIM_JTAG_SC__H

#include "Or1ksimIntrSC.h"


// ----------------------------------------------------------------------------
//! SystemC module class wrapping Or1ksim ISS with temporal decoupling and
//! external interrupts.

//! Provides signals for the interrupts and additional threads sensitive to
//! the interrupt inputs. All other functionality comes from the base class,
//! Or1ksimDecoupSC::.
// ----------------------------------------------------------------------------
class Or1ksimJtagSC
  : public Or1ksimIntrSC
{
public:

  // Constructor
  Or1ksimJtagSC( sc_core::sc_module_name  name,
		 const char              *configFile,
		 const char              *imageFile );

  // The JTAG transactional API
  void  reset (sc_core::sc_time &delay);

  void  shiftIr (unsigned char    *jreg,
		 int               numBits,
		 sc_core::sc_time &delay);

  void  shiftDr (unsigned char    *jreg,
		 int               numBits,
		 sc_core::sc_time &delay);

  
protected:

  // Thread which will run the model, which adds a mutex to prevent calling
  // the underlying ISS when the JTAG thread is active.
  virtual void  run();


private:

  //! SystemC mutex controlling access to the underlying ISS.
  sc_core::sc_mutex  or1ksimMutex;


};	/* Or1ksimJtagSC() */


#endif	// OR1KSIM_JTAG_SC__H

// EOF
