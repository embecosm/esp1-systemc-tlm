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


// Static utility routines (for easy C linkage!) that will call the class
// callback routines. In theory this ought to be possible by using member
// pointers to functions. However given the external linkage is to C and not
// C++, this way is much safer!

static unsigned long int  staticReadCallback( void              *instancePtr,
					      unsigned long int  addr,
					      unsigned long int  mask )
{
  return ((Or1ksimSC *)instancePtr)->readCallback( addr, mask );

}	// staticReadCallback()


static void  staticWriteCallback( void              *instancePtr,
				  unsigned long int  addr,
				  unsigned long int  mask,
				  unsigned long int  wdata )
{
  ((Or1ksimSC *)instancePtr)->writeCallback( addr, mask, wdata );

}	// staticWriteCallback()


// Constructor

SC_HAS_PROCESS( Or1ksimSC );

Or1ksimSC::Or1ksimSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  sc_module( name )
  // dataIni( "data_initiator" )
{
  or1ksim_init( configFile, imageFile, this, staticReadCallback,
		staticWriteCallback );

  // Thread to run the ISS

  SC_THREAD( run );

}	/* Or1ksimSC() */


// Run the ISS forever

void
Or1ksimSC::run()
{
  (void)or1ksim_run( -1LL );

}	// Or1ksimSC()


// Static method, which identifies the endianism of the model

bool
Or1ksimSC::isLittleEndian()
{
  return (1 == or1ksim_is_le());

}	// or1ksimIsLe()


// Take a read from the simulator - this is the entry point used by the
// static callback.

unsigned long int
Or1ksimSC::readCallback( unsigned long int  addr,
			 unsigned long int  mask )
{
  tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload();

  // Set up the command and address

  trans->set_read();
  trans->set_address( (const sc_dt::uint64)addr );

  // Set up the size (always 4 bytes) and allocate a suitable data pointer

  unsigned char *dataPtr = new unsigned char[4];

  trans->set_data_length( 4 );
  trans->set_data_ptr( dataPtr );

  // Set up the byte enable mask (always 4 bytes)

  unsigned char *byteMask = new unsigned char[4];

  (void)memcpy( byteMask, &mask, 4 );

  trans->set_byte_enable_length( 4 );
  trans->set_byte_enable_ptr( byteMask );

  // Send it

  sc_core::sc_time  t = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  dataIni->b_transport( *trans, t );

  // The result should be in the data. Copy it for return.

  unsigned long int  res;
  (void)memcpy( &res, dataPtr, 4 );

  // Free up the memory

  delete [] byteMask;
  delete [] dataPtr;
  delete trans;

  return  res;

}	// readCallback()


// Take a write from the simulator - this is the entry point used by the
// static callback.

void
Or1ksimSC::writeCallback( unsigned long int  addr,
			  unsigned long int  mask,
			  unsigned long int  wdata )
{
  tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload();

  // Set up the command and address

  trans->set_write();
  trans->set_address( (const sc_dt::uint64)addr );

  // Set up the size (always 4 bytes) and allocate a suitable data pointer

  unsigned char *dataPtr = new unsigned char[4];

  (void)memcpy( dataPtr, (unsigned char *)(&wdata), 4 );

  trans->set_data_length( 4 );
  trans->set_data_ptr( dataPtr );

  // Set up the byte enable mask (always 4 bytes)

  unsigned char *byteMask = new unsigned char[4];

  (void)memcpy( byteMask, &mask, 4 );

  trans->set_byte_enable_length( 4 );
  trans->set_byte_enable_ptr( byteMask );

  // Send it

  sc_core::sc_time  t = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  dataIni->b_transport( *trans, t );

  // Free up the memory

  delete [] byteMask;
  delete [] dataPtr;
  delete trans;

}	// writeCallback()


// EOF
