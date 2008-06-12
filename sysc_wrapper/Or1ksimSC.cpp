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

// Implementation of main SystemC wrapper for the OSCI SystemC wrapper project

// $Id$


#include "Or1ksimSC.h"


// Static utility routine (for easy C linkage!) that will call the class
// callback routine. In theory this ought to be possible by using member
// pointers to functions. However given the external linkage is to C and not
// C++, this way is much safer!

static unsigned long int  staticIoCallback( void              *instancePtr,
					    enum or1ksim_cb    code,
					    unsigned long int  addr,
					    unsigned long int  wdata )
{
  ((Or1ksimSC *)instancePtr)->ioCallback( code, addr, wdata );

}	// staticIoCallback()


// Constructor

SC_HAS_PROCESS( Or1ksimSC );

Or1ksimSC::Or1ksimSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  sc_module( name )
  // dataIni( "data_initiator" )
{
  or1ksim_init( configFile, imageFile, staticIoCallback, this );

  // Thread to run the ISS

  SC_THREAD( run );

}	/* Or1ksimSC() */


// Run the ISS forever

void
Or1ksimSC::run()
{
  (void)or1ksim_run( -1LL );

}	// Or1ksimSC()


// Take a callback from the simulator - this is the entry point used by the
// static callback.

unsigned long int
Or1ksimSC::ioCallback( enum or1ksim_cb    code,
		       unsigned long int  addr,
		       unsigned long int  wdata )
{
  tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload();

  /* Set up the command */

  switch( code ) {

  case OR1KSIM_CB_BYTE_R:
  case OR1KSIM_CB_HW_R:
  case OR1KSIM_CB_WORD_R:

    trans->set_command( tlm::TLM_READ_COMMAND );
    break;
    
  case OR1KSIM_CB_BYTE_W:
  case OR1KSIM_CB_HW_W:
  case OR1KSIM_CB_WORD_W:

    trans->set_command( tlm::TLM_WRITE_COMMAND );
    break;
  }

  /* Set the address */

  trans->set_address( (const sc_dt::uint64)addr );

  /* Set up the size and allocate a suitable data pointer */

  unsigned char *dataPtr;

  switch( code ) {

  case OR1KSIM_CB_BYTE_R:
  case OR1KSIM_CB_BYTE_W:

    dataPtr = new unsigned char[1];
    trans->set_data_length( 1 );
    break;
    
  case OR1KSIM_CB_HW_R:
  case OR1KSIM_CB_HW_W:

    dataPtr = new unsigned char[2];
    trans->set_data_length( 2 );
    break;
    
  case OR1KSIM_CB_WORD_R:
  case OR1KSIM_CB_WORD_W:

    dataPtr = new unsigned char[4];
    trans->set_data_length( 4 );
    break;
  } 

  trans->set_data_ptr( dataPtr );

  /* For writes, initialize the data. */

  switch( code ) {

  case OR1KSIM_CB_BYTE_W:

    if( tlm::host_has_little_endianness()) {
      memcpy( dataPtr, (unsigned char *)(&wdata), 1 );
    }
    else {
      memcpy( dataPtr, (unsigned char *)(&wdata + 3), 1 );
    }
    break;
    
  case OR1KSIM_CB_HW_W:

    if( tlm::host_has_little_endianness()) {
      memcpy( dataPtr, (unsigned char *)(&wdata), 2 );
    }
    else {
      memcpy( dataPtr, (unsigned char *)(&wdata + 2), 2 );
    }
    break;
    
  case OR1KSIM_CB_WORD_W:

    memcpy( dataPtr, (unsigned char *)(&wdata), 4 );
    break;
  } 

  // Send it

  sc_core::sc_time  t = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  dataIni->b_transport( *trans, t );

  free( trans );

  return 0;

}	// ioCallback()


// EOF
