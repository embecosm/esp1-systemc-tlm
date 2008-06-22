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

#include <stdint.h>

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "or1ksim.h"


//! SystemC module class wrapping Or1ksim ISS

//! Provides a single thread (::run) which runs the underlying Or1ksim ISS.

class Or1ksimSyncSC
: public sc_core::sc_module
{
 public:

  // Constructor

  Or1ksimSyncSC( sc_core::sc_module_name  name,
	     const char              *configFile,
	     const char              *imageFile );

  // Public utilities to return the endianism of the model and the
  // clock rate

  bool               isLittleEndian();
  unsigned long int  getClockRate();

  //! Initiator port for data accesses

  tlm_utils::simple_initiator_socket<Or1ksimSyncSC>  dataBus;


 private:

  // Thread which will run the model

  void  run();

  // I/O upcalls from Or1ksim, with a common synchronized transport utility.

  static unsigned long int  staticReadUpcall( void              *instancePtr,
					      unsigned long int  addr,
					      unsigned long int  mask );

  static void  staticWriteUpcall( void              *instancePtr,
				  unsigned long int  addr,
				  unsigned long int  mask,
				  unsigned long int  wdata );

  uint32_t           readUpcall( sc_dt::uint64  addr,
				 uint32_t       mask );

  void               writeUpcall( sc_dt::uint64  addr,
				  uint32_t       mask,
				  uint32_t       wdata );

  void               syncTrans( tlm::tlm_generic_payload &trans );

  // Timestamp by SystemC and Or1ksim at the last upcall.

  sc_core::sc_time  scLastUpTime;	//!< SystemC time stamp of last upcall
  double            or1kLastUpTime;	//!< Or1ksim time stamp of last upcall

};	/* Or1ksimSyncSC() */


#endif	// OR1KSIM_SYNC_SC__H


// EOF
