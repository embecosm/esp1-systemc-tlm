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
// with SystemC temporal decoupling and interrupt support

// $Id$


#ifndef OR1KSIM_INTR_SC__H
#define OR1KSIM_INTR_SC__H

#include "Or1ksimDecoupSC.h"


#define  NUM_INTR  32

//! SystemC module class wrapping Or1ksim ISS with temporal decoupling and
//! external interrupts.

//! Provides signals for the interrupts and additional threads sensitive to
//! the interrupt inputs. All other functionality comes from the base class,
//! Or1ksimDecoupSC::.

class Or1ksimIntrSC
  : public Or1ksimDecoupSC
{
public:

  Or1ksimIntrSC( sc_core::sc_module_name  name,
		 const char              *configFile,
		 const char              *imageFile );

  //! An array of signals for interrupts to the processor. The Or1ksim ISS has
  //! a built in programmable interrupt controller (PIC), which manages all
  //! these.
  sc_core::sc_signal<bool>  intr[NUM_INTR];

  
private:

  // Method which will handle interrupts.
  void  intrMethod();

};	/* Or1ksimIntrSC() */


#endif	// OR1KSIM_INTR_SC__H

// EOF
