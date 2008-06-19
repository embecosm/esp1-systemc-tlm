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


// $Id$


#ifndef OR1KSIM_SC__H
#define OR1KSIM_SC__H

#include <stdint.h>

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "or1ksim.h"

//! Module class for the Or1ksim ISS

class Or1ksimSC
: public sc_core::sc_module
{
 public:

  // Constructor

  Or1ksimSC( sc_core::sc_module_name  name,
	     const char              *configFile,
	     const char              *imageFile );

  // Thread that will run the model

  void  run();

  // I/O upcalls from Or1ksim, with a common synchronized transport utility.

  uint32_t           readUpcall( sc_dt::uint64  addr,
				 uint32_t       mask );

  void               writeUpcall( sc_dt::uint64  addr,
				  uint32_t       mask,
				  uint32_t       wdata );

  void               syncTrans( tlm::tlm_generic_payload &trans );

  // Static utilities to return the endianism of the model and the clock rate

  static bool               isLittleEndian();
  static unsigned long int  getClockRate();

  // Initiator port for data accesses (no off chip instructions for now)

  tlm_utils::simple_initiator_socket<Or1ksimSC>  dataIni;

  // Timestamp by SystemC and Or1ksim at the last upcall.

  sc_core::sc_time  scLastUpTime;
  double            or1kLastUpTime;

};	/* Or1ksimSC() */


#endif	// OR1KSIM_SC__H


// EOF
