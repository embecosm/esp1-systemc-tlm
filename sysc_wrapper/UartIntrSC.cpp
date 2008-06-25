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

// Implementation of 16450 UART Synchronous SystemC module.

// $Id$


#include "UartIntrSC.h"


SC_HAS_PROCESS( UartIntrSC );

//! Custom constructor for the UART module

//! Calls the parent constructor and marks the size of interrupt FIFO, then
//! sets up the thread handling interrupt generation.

//! @param name             The SystemC module name, passed to the parent
//!                         constructor
//! @param _clockRate       The external clock rate, passed to the parent
//!                         consturctor
//! @param _isLittleEndian  The model endianism

UartIntrSC::UartIntrSC( sc_core::sc_module_name  name,
			unsigned long int        _clockRate,
			bool                     _isLittleEndian ) :
  UartDecoupSC( name, _clockRate, _isLittleEndian ),
  intrQueue( 1 )
{

  SC_THREAD( intrThread );

}	/* UartIntrSC() */


//! SystemC thread driving the interrupt signal

//! If true is read, assert the interrupt, otherwise deassert. Needed since a
//! single process must drive a signal.

void
UartIntrSC::intrThread()
{
  intr.write( false );		// Clear interrupt on startup

  while( true ) {
    intr.write( intrQueue.read() );
  }
 }	// intrThread()


//! Generate an interrupt

//! Updated version from the base class, UartSC::genIntr(), which triggers a
//! write to assert the interrupt signal port if required.

//! @param ierFlag  Indicator of which interrupt is to be cleared (as IER bit).

void
UartIntrSC::genIntr( unsigned char  ierFlag )
{
  if( isSet( regs.ier, ierFlag )) {
    set( intrPending, ierFlag );	// Mark this interrupt as pending.

    (void)setIntrFlags();		// Show highest priority

    clr( regs.iir, UART_IIR_IPEND );	// Mark (0 = pending) and queue
    intrQueue.write( true );
  }
}	// genIntr()


//! Clear an interrupt

//! Updated version from the base class, UartSC::clrIntr(), which triggers a
//! write to deassert the interrupt signal port if required.

//! @param ierFlag  Indicator of which interrupt is to be cleared (as IER bit).

void
UartIntrSC::clrIntr( unsigned char ierFlag )
{
  clr( intrPending, ierFlag );

  if( !setIntrFlags()) {		// Deassert if none left
    set( regs.iir, UART_IIR_IPEND );	// 1 = not pending
    intrQueue.write( false );
  }
}	// clrIntr()



// EOF
