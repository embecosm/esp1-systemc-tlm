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
// with SystemC time synchronization


// $Id$


#ifndef OR1KSIM_SYNC_SC__H
#define OR1KSIM_SYNC_SC__H

#include "Or1ksimExtSC.h"


//! SystemC module class wrapping Or1ksim ISS with synchronized timing

//! Provides a single thread (::run) which runs the underlying Or1ksim
//! ISS. Derived from the earlier Or1ksimExtSC class.

class Or1ksimSyncSC
: public Or1ksimExtSC
{
 public:

  // Constructor

  Or1ksimSyncSC( sc_core::sc_module_name  name,
		 const char              *configFile,
		 const char              *imageFile );

  // Public utility to return the clock rate

  unsigned long int  getClockRate();


 protected:

  // The common thread to make the transport calls. This has static timing. It
  // will be further modified in later calls to add termporal decoupling.

  virtual void  doTrans( tlm::tlm_generic_payload &trans );


 private:

  // Timestamp by SystemC and Or1ksim at the last upcall. Used only by this
  // class

  sc_core::sc_time  scLastUpTime;	//!< SystemC time stamp of last upcall
  double            or1kLastUpTime;	//!< Or1ksim time stamp of last upcall

};	/* Or1ksimSyncSC() */


#endif	// OR1KSIM_SYNC_SC__H


// EOF
