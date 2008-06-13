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

  // I/O callbacks

  unsigned long int  readCallback( unsigned long int  mask,
				   unsigned long int  addr );

  void               writeCallback( unsigned long int  addr,
				    unsigned long int  mask,
				    unsigned long int  wdata );

  // Static utility to return the endianism of the model

  static bool  isLittleEndian();

  // Initiator port for data accesses (no off chip instructions for now)

  tlm_utils::simple_initiator_socket<Or1ksimSC>  dataIni;

};	/* Or1ksimSC() */


#endif	// OR1KSIM_SC__H


// EOF
