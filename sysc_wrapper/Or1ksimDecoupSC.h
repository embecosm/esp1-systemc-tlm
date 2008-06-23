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
// with SystemC temporal decoupling


// $Id$


#ifndef OR1KSIM_DECOUP_SC__H
#define OR1KSIM_DECOUP_SC__H

#include "Or1ksimSyncSC.h"
#include "tlm_utils/tlm_quantumkeeper.h"


//! SystemC module class wrapping Or1ksim ISS with temporal decoupling

//! Provides a single thread (::run) which runs the underlying Or1ksim
//! ISS. Derived from the earlier Or1ksimSyncSC class.

class Or1ksimDecoupSC
: public Or1ksimSyncSC
{
 public:

  // Constructor

  Or1ksimDecoupSC( sc_core::sc_module_name  name,
		   const char              *configFile,
		   const char              *imageFile );


 protected:

  // Thread which will run the model, with temporal decoupling.

  virtual void  run();

  // The common thread to make the transport calls. This has temporal
  // decoupling.

  virtual void  doTrans( tlm::tlm_generic_payload &trans );


 private:

  //! TLM 2.0 Quantum keeper for the ISS model thread. Won't be used in any
  //! derived classes.

  tlm_utils::tlm_quantumkeeper  issQk;

};	/* Or1ksimDecoupSC() */


#endif	// OR1KSIM_DECOUP_SC__H


// EOF
