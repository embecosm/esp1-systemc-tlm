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

// Definition of 16450 UART Temporally Decoupled SystemC module.

// $Id$


#ifndef UART_INTR_SC__H
#define UART_INTR_SC__H

#include "UartDecoupSC.h"

//! SystemC module class for a 16450 UART with temporal decoupling and
//! external interrupt.

//! Derived from UartDecoupSC:: to provide a thread which can drive a single
//! sc_signal port whether the interrupt is requested by either the existing
//! UartSyncSC::busThread() thread or UartSC::rxMethod() method.

//! @note Either of the other two processes may want to drive an interrupt,
//!       but only one process can drive a signal, so a separate thread is
//!       created and notified (via a one slot FIFO) by either of the other
//!       threads when it wishes to drive the interrupt.

class UartIntrSC
  : public UartDecoupSC
{
public:

  UartIntrSC( sc_core::sc_module_name  name,
		unsigned long int        _clockRate,
		bool                     _isLittleEndian );

  sc_core::sc_out<bool>  intr;		//!< Interrupt output port


private:

  // A single thread for driving interrupts. Only used in this class

  void  intrThread();

  // Reimplemented utility routines for interrupt handling, which drive the real
  // signal. Will not be reimplemented further.

  void  genIntr( unsigned char  ierFlag );
  void  clrIntr( unsigned char  ierFlag );

  //! A boolean fifo is used to communicate with the interrupt thread, to
  //! ensure that requests to set/clear are taken in the order they are sent.
  //! True to set, false to clear.

  sc_core::sc_fifo<bool>  intrQueue;




};	// UartIntrSC()


#endif	// UART_INTR_SC__H


// EOF
