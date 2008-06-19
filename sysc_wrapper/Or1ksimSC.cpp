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
// C++, this way is much safer! Cast to SystemC fixed width types.

static unsigned long int  staticReadUpcall( void              *instancePtr,
					    unsigned long int  addr,
					    unsigned long int  mask )
{
  Or1ksimSC *classPtr = (Or1ksimSC *)instancePtr;

  return (unsigned long int)classPtr->readUpcall( (sc_dt::uint64)addr,
						  (uint32_t)mask );
}	// staticReadUpcall()


static void  staticWriteUpcall( void              *instancePtr,
				unsigned long int  addr,
				unsigned long int  mask,
				unsigned long int  wdata )
{
  Or1ksimSC *classPtr = (Or1ksimSC *)instancePtr;

  classPtr->writeUpcall( (sc_dt::uint64)addr, (uint32_t)mask, (uint32_t)wdata );

}	// staticWriteUpcall()


// Constructor

SC_HAS_PROCESS( Or1ksimSC );

Or1ksimSC::Or1ksimSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  sc_module( name ),
  dataIni( "data_initiator" )
{
  or1ksim_init( configFile, imageFile, this, staticReadUpcall,
		staticWriteUpcall );

  SC_THREAD( run );		  // Thread to run the ISS

}	/* Or1ksimSC() */


// Run the ISS forever

void
Or1ksimSC::run()
{
  scLastUpTime   = sc_core::sc_time_stamp();
  or1kLastUpTime = or1ksim_time();

  (void)or1ksim_run( -1.0 );

}	// Or1ksimSC()


// Take a read from the simulator - this is the entry point used by the
// static callback.

uint32_t
Or1ksimSC::readUpcall( sc_dt::uint64  addr,
		       uint32_t       mask )
{
  tlm::tlm_generic_payload  trans;
  uint32_t        rdata;		// For the result

  // Set up the payload fields. Assume everything is 4 bytes.

  trans.set_read();
  trans.set_address( addr );

  trans.set_data_length( 4 );
  trans.set_data_ptr( (unsigned char *)&rdata );

  trans.set_byte_enable_length( 4 );
  trans.set_byte_enable_ptr( (unsigned char *)&mask );

  // Synchronize clocks and transport. Then return the result

  syncTrans( trans );
  return  rdata;

}	// readUpcall()


// Take a write from the simulator - this is the entry point used by the
// static callback.

void
Or1ksimSC::writeUpcall( sc_dt::uint64  addr,
			uint32_t       mask,
			uint32_t       wdata )
{
  tlm::tlm_generic_payload  trans;

  // Set up the payload fields. Assume everything is 4 bytes.

  trans.set_write();
  trans.set_address( addr );

  trans.set_data_length( 4 );
  trans.set_data_ptr( (unsigned char *)&wdata );

  trans.set_byte_enable_length( 4 );
  trans.set_byte_enable_ptr( (unsigned char *)&mask );

  // Synchronize clocks and transport.

  syncTrans( trans );

}	// writeUpcall()


// Synchronized transport from the initiator

void
Or1ksimSC::syncTrans( tlm::tlm_generic_payload &trans )
{
  // Synchronize with SystemC for the amount of time that the ISS has used
  // since the last upcall.

  double  or1kGap = or1ksim_time() - or1kLastUpTime;

  scLastUpTime   += sc_core::sc_time( or1kGap, sc_core::SC_SEC );
  or1kLastUpTime += or1kGap;

  wait( scLastUpTime - sc_core::sc_time_stamp());

  // Call the transport, backdating any additional time owed for the
  // transaction.

  sc_core::sc_time  delay = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  dataIni->b_transport( trans, delay );
  scLastUpTime += delay;

//   fprintf( stderr, "%s 0x%02x%02x%02x%02x with mask 0x%02x%02x%02x%02x"
// 	   " from 0x%08lx\n", trans.is_read() ? "Read " : "Write",
// 	   (trans.get_data_ptr())[3], (trans.get_data_ptr())[2],
// 	   (trans.get_data_ptr())[1], (trans.get_data_ptr())[0],
// 	   (trans.get_byte_enable_ptr())[3], (trans.get_byte_enable_ptr())[2],
// 	   (trans.get_byte_enable_ptr())[1], (trans.get_byte_enable_ptr())[0],
// 	   (unsigned long int)(trans.get_address()));

}	// syncTrans()


// Static method, which identifies the endianism of the model

bool
Or1ksimSC::isLittleEndian()
{
  return (1 == or1ksim_is_le());

}	// or1ksimIsLe()


// Static method to return the ISS clock rate

unsigned long int
Or1ksimSC::getClockRate()
{
  return or1ksim_clock_rate();

}	// getClockRate()

// EOF
