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

// Implementation of the simple logger object.

// $Id$


#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "LoggerSC.h"


// Constructor, which registers the transport call

LoggerSC::LoggerSC( sc_core::sc_module_name  name ) :
  sc_module( name )
{
  loggerPort.register_b_transport( this, &LoggerSC::loggerReadWrite );

}	// Or1ksimSC()


// The blocking transport routine for the CPU facing socket. This is only for
// single word read/write

void
LoggerSC::loggerReadWrite( tlm::tlm_generic_payload &payload,
			   sc_core::sc_time         &delayTime )
{
  // Break out the address, mask and data pointer.

  tlm::tlm_command   comm    = payload.get_command();
  sc_dt::uint64      addr    = payload.get_address();
  unsigned char     *maskPtr = payload.get_byte_enable_ptr();
  unsigned char     *dataPtr = payload.get_data_ptr();

  // Which command?

  const char *commStr;

  switch( comm ) {
  case tlm::TLM_READ_COMMAND:   commStr = "Read";   break;
  case tlm::TLM_WRITE_COMMAND:  commStr = "Write";  break;
  case tlm::TLM_IGNORE_COMMAND: commStr = "Ignore"; break;
  }

  // Log the lot (data only if its a write)

  std::cout << "Logging" << std::endl;
  std::cout << "  Command:      "   << commStr << std::endl;
  std::cout << "  Address:      0x" << std::setw( 8 ) << std::setfill( '0' )
	    <<std::hex << (uint64_t)addr << std::endl;
  std::cout << "  Byte enables: 0x" << std::setw( 8 ) << std::setfill( '0' )
	    <<std::hex << *((uint32_t *)maskPtr) << std::endl;

  if( tlm::TLM_WRITE_COMMAND == comm ) {
    std::cout << "  Data:         0x" << std::setw( 8 ) << std::setfill( '0' )
	      <<std::hex << *((uint32_t *)dataPtr) << std::endl;
  }

  std::cout << "  Delay:        " << std::setprecision( 9 ) << std::fixed
	    << delayTime.to_seconds() << "s" << std::endl;
  std::cout << std::endl;

  // Set a response before returning

  payload.set_response_status( tlm::TLM_OK_RESPONSE );

}	// loggerReadWrite()

// EOF
