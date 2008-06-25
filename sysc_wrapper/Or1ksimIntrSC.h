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

// Definition of main SystemC wrapper for the OSCI SystemC wrapper project
// with SystemC temporal intrling and interrupt support


// $Id$


#ifndef OR1KSIM_INTR_SC__H
#define OR1KSIM_INTR_SC__H

#include "Or1ksimDecoupSC.h"


#define  NUM_INTR  32

//! SystemC module class wrapping Or1ksim ISS with temporal decoupling and
//! interrupts.

//! Provides signals for the interrupts and additional threads sensitive to
//! the interrupt inputs. All other functionality comes from the base class,
//! Or1ksimDecoupSC::.

class Or1ksimIntrSC
  : public Or1ksimDecoupSC
{
public:

  // Constructor.

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
