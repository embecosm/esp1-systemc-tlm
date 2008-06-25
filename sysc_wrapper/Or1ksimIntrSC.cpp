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
// with SystemC temporal intrling

// $Id$


#include "Or1ksimIntrSC.h"


//! Custom constructor for the Or1ksimIntrSC SystemC module with interrupt
//! handling

//! Calls the constructor of the base class, Or1ksimSyncSC::, then sets up a
//! thread for the signal ports.

//! @param name           SystemC module name
//! @param configFile     Config file for the underlying ISS
//! @param imageFile      Binary image to run on the ISS


SC_HAS_PROCESS( Or1ksimIntrSC );

Or1ksimIntrSC::Or1ksimIntrSC ( sc_core::sc_module_name  name,
			       const char              *configFile,
			       const char              *imageFile ) :
  Or1ksimDecoupSC( name, configFile, imageFile )
{
  int  i;

  SC_METHOD( intrMethod );
  for( i = 0 ; i < NUM_INTR ; i++ ) {
    sensitive << intr[i].posedge_event();
  }
  dont_initialize();

}	/* Or1ksimIntrSC() */


//! Method to handle interrupt triggers.

//! Triggered statically on posedge write to any interrupt. Identifies the
//! generating trigger and signals the underlying Or1ksim ISS.

void
Or1ksimIntrSC::intrMethod()
{
  int  i;

  for( i = 0 ; i < NUM_INTR ; i++ ) {
    if( intr[i].event()) {
      or1ksim_interrupt( i );
    }
  }

}	// intrMethod()


// EOF
