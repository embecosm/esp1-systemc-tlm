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

// Implementation of Data reporter object for the OSCI SystemC wrapper project

// $Id$


#include <iostream>

#include "DataReporterSC.h"


// Constructor

SC_HAS_PROCESS( DataReporterSC );

DataReporterSC::DataReporterSC ( sc_core::sc_module_name  name ) :
  sc_module( name )
{
  // Register the blocking transport method

  dataTar.register_b_transport( this, &DataReporterSC::dataRepBlkTrans);

}	/* DataReporterSC() */


// The blocking transport routine.

void
DataReporterSC::dataRepBlkTrans( tlm::tlm_generic_payload &payload,
				 sc_core::sc_time         &delay_time )
{
  std::cout << "Reporting" << std::endl;

  // Which command?

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:
    std::cout << "  Read" << std::endl;
    break;

  case tlm::TLM_WRITE_COMMAND:
    std::cout << "  Write" << std::endl;
    break;

  case tlm::TLM_IGNORE_COMMAND:
    std::cout << "  Ignore" << std::endl;
    break;
  }

  // What address?

  std::cout << "  Address: " << std::hex << payload.get_address() << std::endl;

  // Data if writing

  if( payload.is_write()) {

    int            l   = payload.get_data_length();
    unsigned char *dat = payload.get_data_ptr();

    std::cout << "  Data size: " << l << std::endl;

    for( int i = 0 ; i < l ; i++ ) {
      std::cout << "    dat[" << i << "] = " << std::hex << (int)(dat[i])
		<< std::endl;
    }
  }

  // Byte mask

  int            l    = payload.get_byte_enable_length();
  unsigned char *mask = payload.get_byte_enable_ptr();

  std::cout << "  Byte mask size: " << l << std::endl;

  for( int i = 0 ; i < l ; i++ ) {
    std::cout << "    mask[" << i << "] = " << std::hex << (int)(mask[i])
	      << std::endl;
  }

  // All done

  std::cout << std::endl ;

  // Set the reponse

  payload.set_response_status( tlm::TLM_OK_RESPONSE );
    
}	// dataRepBlkTrans()

// EOF
