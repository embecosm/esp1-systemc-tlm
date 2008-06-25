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
//   SC_THREAD( intr0Thread );
//   sensitive << intr0.pos();
//   dont_initialize();

//   SC_THREAD( intr1Thread );
//   sensitive << intr1.pos();
//   dont_initialize();

  SC_METHOD( intr2Thread );
  sensitive << intr2.pos();
  dont_initialize();

//   SC_THREAD( intr3Thread );
//   sensitive << intr3.pos();
//   dont_initialize();

}	/* Or1ksimIntrSC() */


//! The SystemC thread listening for interrupt 0

//! Triggers the underlying interrupt system.

// void
// Or1ksimIntrSC::intr0Thread()
// {
//   printf( "Got interrupt 0\n" );
//   or1ksim_interrupt( 0 );

// }	// intrThread()


//! The SystemC thread listening for interrupt 1

//! Triggers the underlying interrupt system.

// void
// Or1ksimIntrSC::intr1Thread()
// {
//   printf( "Got interrupt 1\n" );
//   or1ksim_interrupt( 1 );

// }	// intrThread()


//! The SystemC thread listening for interrupt 2

//! Triggers the underlying interrupt system.

void
Or1ksimIntrSC::intr2Thread()
{
  or1ksim_interrupt( 2 );

}	// intrThread()


//! The SystemC thread listening for interrupt 3

//! Triggers the underlying interrupt system.

// void
// Or1ksimIntrSC::intr3Thread()
// {
//   printf( "Got interrupt 3\n" );
//   or1ksim_interrupt( 3 );

// }	// intrThread()


// EOF
