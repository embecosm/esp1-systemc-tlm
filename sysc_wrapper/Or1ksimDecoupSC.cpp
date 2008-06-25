// ----------------------------------------------------------------------------

//                  CONFIDENTIAL AND PROPRIETARY INFORMATION
//                  ========================================

// Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.

// This file contains confidential and proprietary information of Embecosm and
// is protected by copyright, trade secret and other regional, national and
// international laws, and may be embodied in patents issued or pending.

// Receipt or possession of this file does not convey any rights to use,
// reproduce, disclose its contents, or to manufacture, or sell anything it may
// describe.

// Reproduction, disclosure or use without specific written authorization of
// Embecosm is strictly forbidden.

// Reverse engineering is prohibited.

// ----------------------------------------------------------------------------

// Implementation of main SystemC wrapper for the OSCI SystemC wrapper project
// with SystemC temporal decoupling

// $Id$


#include "Or1ksimDecoupSC.h"


//! Custom constructor for the Or1ksimDecoupSC SystemC module

//! Calls the constructor of the base class, Or1ksimSyncSC::, then initializes
//! the local quantum keeper.

//! @param name           SystemC module name
//! @param configFile     Config file for the underlying ISS
//! @param imageFile      Binary image to run on the ISS


Or1ksimDecoupSC::Or1ksimDecoupSC ( sc_core::sc_module_name  name,
				   const char              *configFile,
				   const char              *imageFile ) :
  Or1ksimSyncSC( name, configFile, imageFile )
{
  tlm::tlm_global_quantum &refTgq = tlm::tlm_global_quantum::instance();
  issQk.set_global_quantum( refTgq.get() );
  issQk.reset();				// Zero local time offset

}	/* Or1ksimDecoupSC() */


//! The SystemC thread running the underlying ISS

//! This version of the thread uses the local quantum keeper to ensure that we
//! never run ahead more than the global quantum

//! Handling the timing is difficult, since there are two points where timing
//! can advance, here and in the read/write upcalls. Either may advance the
//! local time, and either may affect the amount of time more that the ISS can
//! run until it hits the end of the quantum.

//! To facilitate this several routines are aded to the library. Running is
//! split into two routines, one to set the ISS time point for the end of the
//! run (which may be changed mid-run) and one to specify a run until that end
//! point is reached.

//! Two routines are added, so the ISS can record a timing start point and
//! report back its value. This means that whichever routine (this thread or
//! the upcall) gets control next can find out how much execution the ISS has
//! completed.

//! There are upcalls for read and write to peripherals (@see
//! ::staticReadUpcall(), ::staticWriteUpcall, ::readUpcall(),
//! ::writeUpcall()), which provide opportunities for the thread to yield, and
//! so not block the simulation.

void
Or1ksimDecoupSC::run()
{
  tlm::tlm_global_quantum &refTgq = tlm::tlm_global_quantum::instance();

  while( true ) {
    sc_core::sc_time  timeLeft =
      refTgq.compute_local_quantum() - issQk.get_local_time();

    // Mark the start of the ISS timing point, set a desired run duration and
    // run for that duration (either of these may be changed by the read/write
    // upcalls). On return advance the local time according to how much time
    // has been used since the last ISS timing point, and if necessary
    // synchronize.

    (void)or1ksim_run( timeLeft.to_seconds() );

    issQk.inc( sc_core::sc_time( or1ksim_get_time_period(), sc_core::SC_SEC ));
    or1ksim_set_time_point();

    // Sync if needed

    if( issQk.need_sync() ) {
      issQk.sync();
    }
  }

}	// Or1ksimSC()


//! Temporally Decoupled TLM transport to the target

//! The time used by the ISS is added to the local time. This should not
//! itself require synchronization (if that time had been reached, control
//! would have returned to the Or1ksimDecoupSC::run() thread.

//! @param trans  The transaction payload

void
Or1ksimDecoupSC::doTrans( tlm::tlm_generic_payload &trans )
{
  issQk.inc( sc_core::sc_time( or1ksim_get_time_period(), sc_core::SC_SEC ));
  or1ksim_set_time_point();

  // Call the transport. Since this is a temporally decoupled model, the
  // current local time is set in the delay.

  sc_core::sc_time  delay = issQk.get_local_time();
  dataBus->b_transport( trans, delay );
  issQk.set( delay );			// Updated

  // This may have pushed us into needing to synchronize.

  if( issQk.need_sync() ) {
    issQk.sync();
  }

  // Reset the time the ISS is allowed to continue running

  tlm::tlm_global_quantum &refTgq = tlm::tlm_global_quantum::instance();
  sc_core::sc_time  timeLeft      =
    refTgq.compute_local_quantum() - issQk.get_local_time();

  or1ksim_reset_duration ( timeLeft.to_seconds() );

}	// doTrans()


// EOF