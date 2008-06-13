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

// Definition of Data reporter object for the OSCI SystemC wrapper project

// $Id$


#ifndef DATA_REPORTER_SC__H
#define DATA_REPORTER_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "or1ksim.h"

//! Module class for the Data reporter.

class DataReporterSC
: public sc_core::sc_module
{
 public:

  // Constructor

  DataReporterSC( sc_core::sc_module_name  name );

  // Transport method

  void  dataRepBlkTrans( tlm::tlm_generic_payload &payload,
			 sc_core::sc_time         &delay_time );

  // Target port for data accesses to be reported
  
  tlm_utils::simple_target_socket<DataReporterSC>  dataTar;	// Target port

};	/* DataReporterSC() */


#endif	// DATA_REPORTER_SC__H


// EOF
