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
// with SystemC time synchronization

// $Id$


#include "Or1ksimSyncSC.h"


//! Custom constructor for the Or1ksimSyncSC SystemC module

//! This just calls the constructor of the base class, Or1ksimExtSC::.

//! @param name        SystemC module name
//! @param configFile  Config file for the underlying ISS
//! @param imageFile   Binary image to run on the ISS


Or1ksimSyncSC::Or1ksimSyncSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  Or1ksimExtSC( name, configFile, imageFile )
{
}	/* Or1ksimSyncSC() */


//! Synchronized TLM transport to the target

//! The module synchronizes with SystemC for the time consumed by the ISS,
//! then calls the blocking transport routine for the initiator socket (@see
//! Or1ksimSC::dataBus). SystemC time is adjusted for any delay due to the
//! external entity.

//! @param trans  The transaction payload

void
Or1ksimSyncSC::doTrans( tlm::tlm_generic_payload &trans )
{
  // Synchronize with SystemC for the amount of time that the ISS has used
  // since the last upcall.

  double  or1kGap = or1ksim_time() - or1kLastUpTime;

  scLastUpTime   += sc_core::sc_time( or1kGap, sc_core::SC_SEC );
  or1kLastUpTime += or1kGap;

  wait( scLastUpTime - sc_core::sc_time_stamp());

  // Call the transport. Since this is a synchronous model, the target should
  // have synchronized, and no additional delay be added on return. However we
  // should update the scLastUpTime with a new time stamp.

  sc_core::sc_time  delay = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  dataBus->b_transport( trans, delay );
  scLastUpTime = sc_core::sc_time_stamp();

}	// doTrans()


//! Get the clock rate of the underlying Or1ksim ISS

//! Public utility to allow other modules to identify the ISS clock rate

//! @return  The clock rate in Hz of the underlying Or1ksim ISS

unsigned long int
Or1ksimSyncSC::getClockRate()
{
  return or1ksim_clock_rate();

}	// getClockRate()


// EOF
