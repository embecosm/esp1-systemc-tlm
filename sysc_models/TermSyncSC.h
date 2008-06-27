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

// Definition of a xterm terminal emulator SystemC module class with
// synchronous timing

// $Id$


#ifndef TERM_SYNC_SC__H
#define TERM_SYNC_SC__H

#include "TermSC.h"


//! SystemC module class for the Terminal with synchronized timing

//! Talks to the outside world via two systemC FIFOs. Any data coming in has
//! already been delayed (to represent the baud rate wire delay) via the
//! UART. Any data we send out is similarly delayed.

class TermSyncSC
  : public TermSC
{
public:

  TermSyncSC( sc_core::sc_module_name  name,
	      unsigned long int        baudRate );


private:

  // Reimplemented thread with timing to model the baud rate delay. Will not
  // be modified further in derived classes.
  void  xtermThread();

  //!< Baud rate delay to send a character
  sc_core::sc_time  charDelay;

};	/* TermSyncSC() */


#endif	// TERM_SYNC_SC__H

// EOF
