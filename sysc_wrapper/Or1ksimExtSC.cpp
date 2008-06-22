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

// $Id$


#include "Or1ksimExtSC.h"


//! Custom constructor for the Or1ksimExtSC extended SystemC module

//! Just calls the parent contstructor.

//! @param name        SystemC module name
//! @param configFile  Config file for the underlying ISS
//! @param imageFile   Binary image to run on the ISS


Or1ksimExtSC::Or1ksimExtSC ( sc_core::sc_module_name  name,
			     const char              *configFile,
			     const char              *imageFile ) :
  Or1ksimSC( name, configFile, imageFile )
{
 }	// Or1ksimExtSC()


//! Identify the endianism of the underlying Or1ksim ISS

//! Public utility to allow other modules to identify the model endianism

//! @return  True if the Or1ksim ISS is little endian

bool
Or1ksimExtSC::isLittleEndian()
{
  return (1 == or1ksim_is_le());

}	// or1ksimIsLe()


//! Extended TLM transport to the target

//! Calls the blocking transport routine for the initiator socket (@see
//! ::dataBus).

//! This version adds a zero time delay call to wait() so that the thread will
//! yield in an untimed model.

//! @param trans  The transaction payload

void
Or1ksimExtSC::doTrans( tlm::tlm_generic_payload &trans )
{
  sc_core::sc_time  dummyDelay = sc_core::sc_time( 0.0, sc_core::SC_SEC );

  // Call the transport and wait for no time, which allows the thread to yield
  // and others to get a look in!

  dataBus->b_transport( trans, dummyDelay );
  wait( sc_core::sc_time( 0.0, sc_core::SC_SEC ));

}	// doTrans()


// EOF
