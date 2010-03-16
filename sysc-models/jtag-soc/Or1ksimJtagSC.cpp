// Wrapper for Or1ksim with interrupts and JTAG module implementation

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

#include "Or1ksimJtagSC.h"


using sc_core::SC_SEC;
using sc_core::sc_module_name;
using sc_core::sc_time;


// ----------------------------------------------------------------------------
//! Custom constructor for the Or1ksimJtagSC SystemC module

//! Although there is a new thread of control to handle JTAG, it is not
//! declared here. Since we are (for JTAG) a TLM target, it will be the thread
//! of the initiator socket.

//! In this version we are not a true TLM 2.0 socket. Instead we offer three
//! transactional interfaces, ::shiftIr, ::shiftDr and ::reset ().

//! We must initialize the Mutex to be unlocked.

//! @param name           SystemC module name
//! @param configFile     Config file for the underlying ISS
//! @param imageFile      Binary image to run on the ISS
// ----------------------------------------------------------------------------
Or1ksimJtagSC::Or1ksimJtagSC (sc_module_name  name,
			      const char     *configFile,
			      const char     *imageFile) :
  Or1ksimIntrSC (name, configFile, imageFile)
{
  // Unlock the Mutex
  or1ksimMutex.unlock ();

}	/* Or1ksimJtagSC() */


// ----------------------------------------------------------------------------
//! The SystemC thread running the underlying ISS

//! This version uses temporal decoupling and is based on the version in the
//! base class, Or1ksimDecoupSC::run ().

//! This version adds a mutex to lock access to the underlying ISS. The JTAG
//! thread may not simultaneously access this.
// ----------------------------------------------------------------------------
void
Or1ksimJtagSC::run ()
{
  while( true ) {
    // Compute how much time left in this quantum
    sc_time  timeLeft =
      tgq->compute_local_quantum () - issQk.get_local_time ();

    // Reset the ISS time point.
    or1ksim_set_time_point ();

    // Run for (up to) the remainder of this quantum. Surround the call by a
    // mutex lock.
    or1ksimMutex.lock ();
    (void)or1ksim_run (timeLeft.to_seconds ());
    or1ksimMutex.unlock ();

    // Increment local time by the amount consumed.
    issQk.inc (sc_time (or1ksim_get_time_period(), SC_SEC));

    // If we halted before the end of the ISS period, that is presumably
    // because we have stalled. In which case we should immediately sync to
    // allow a potential JTAG thread to run. Otherwise we should only sync if
    // we have reached the end of the quantum. Except that must be the
    // case. So we don't need to call need_sync (), we can just sync here.
    issQk.sync ();
  }

}	// Or1ksimSC ()


// ----------------------------------------------------------------------------
//! Transaction to request reset of the JTAG interface.

//! Dummy code for now. Just claims and releases the mutex.

//! @param[in,out] delay  The delay so far in the transaction (in) and the
//!                       total delay after the transaction (out).
// ----------------------------------------------------------------------------
void
Or1ksimJtagSC::reset (sc_time &delay)
{
  or1ksimMutex.lock ();
  or1ksimMutex.unlock ();

}	// reset ()


// ----------------------------------------------------------------------------
//! Transaction to shift a register through the JTAG instruction register

//! Dummy code for now. Just claims and releases the mutex.

//! @param[in,out] jreg   The JTAG register to shift in and out.
//! @param[in,out] delay  The delay so far in the transaction (in) and the
//!                       total delay after the transaction (out).
// ----------------------------------------------------------------------------
void
Or1ksimJtagSC::shiftIr (unsigned char *jreg,
			sc_time       &delay)
{
  or1ksimMutex.lock ();
  or1ksimMutex.unlock ();

}	// shiftIr ()


// ----------------------------------------------------------------------------
//! Transaction to shift a register through the JTAG data register

//! Dummy code for now. Just claims and releases the mutex.

//! @param[in,out] jreg   The JTAG register to shift in and out.
//! @param[in,out] delay  The delay so far in the transaction (in) and the
//!                       total delay after the transaction (out).
// ----------------------------------------------------------------------------
void
Or1ksimJtagSC::shiftDr (unsigned char *jreg,
			sc_time       &delay)
{
  or1ksimMutex.lock ();
  or1ksimMutex.unlock ();

}	// shiftDr ()


// EOF
