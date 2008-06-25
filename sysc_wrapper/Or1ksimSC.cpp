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

// Implementation of the basic SystemC wrapper for Or1ksim

// $Id$


#include "Or1ksimSC.h"


SC_HAS_PROCESS( Or1ksimSC );

//! Custom constructor for the Or1ksimSC SystemC module

//! Initializes the underlying Or1ksim ISS, passing in the configuration and
//! image files, a pointer to this class instance and pointers to the static
//! upcall routines (@see ::staticReadUpcall() and ::staticWriteUpcall()).

//! Declares the main SystemC thread, ::run().

//! @param name        SystemC module name
//! @param configFile  Config file for the underlying ISS
//! @param imageFile   Binary image to run on the ISS


Or1ksimSC::Or1ksimSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  sc_module( name ),
  dataBus( "data_initiator" )
{
  or1ksim_init( configFile, imageFile, this, staticReadUpcall,
		staticWriteUpcall );

  SC_THREAD( run );		  // Thread to run the ISS

}	// Or1ksimSC()


//! The SystemC thread running the underlying ISS

//! The thread calls the ISS to run forever.

//! There are upcalls for read and write to peripherals (@see
//! ::staticReadUpcall(), ::staticWriteUpcall, ::readUpcall(),
//! ::writeUpcall()), which provide opportunities for the thread to yield, and
//! so not block the simulation.

void
Or1ksimSC::run()
{
  (void)or1ksim_run( -1.0 );

}	// Or1ksimSC()


//! Static upcall for read from the underlying Or1ksim ISS

//! Static utility routine is used (for easy C linkage!) that will call the
//! class callback routine. Takes standard C types (the underlying ISS is in
//! C) as arguments, but calls the member routine, ::readUpcall(), with fixed
//! width types (from stdint.h).

//! All reads are 32 bits wide, but some bytes may be masked off, to enable
//! byte and half-word accesses.

//! @note In theory this ought to be possible by using member pointers to
//! functions. However given the external linkage is to C and not C++, this way
//! is much safer!

//! @param instancePtr  The pointer to the class member associated with this
//!                     upcall (originally passed to or1ksim_init in the
//!                     constructor, ::Or1ksimSC()).
//! @param addr         The address for the read
//! @param mask         The byte enable mask for the read

//! @return  The value read, cast back to a C type which can hold 32 bits

unsigned long int
Or1ksimSC::staticReadUpcall( void              *instancePtr,
			     unsigned long int  addr,
			     unsigned long int  mask )
{
  Or1ksimSC *classPtr = (Or1ksimSC *)instancePtr;

  return (unsigned long int)classPtr->readUpcall( (sc_dt::uint64)addr,
						  (uint32_t)mask );
}	// staticReadUpcall()


//! Static upcall for write to the underlying Or1ksim ISS

//! Static utility routine is used (for easy C linkage!) that will call the
//! class callback routine. Takes standard C types (the underlying ISS is in
//! C) as arguments, but calls the member routine, ::writeUpcall(), with fixed
//! width types (from stdint.h).

//! All writes are 32 bits wide, but some bytes may be masked off, to enable
//! byte and half-word accesses.

//! @note In theory this ought to be possible by using member pointers to
//! functions. However given the external linkage is to C and not C++, this way
//! is much safer!

//! @param instancePtr  The pointer to the class member associated with this
//!                     upcall (originally passed to or1ksim_init in the
//!                     constructor, ::Or1ksimSC()).
//! @param addr         The address for the write
//! @param mask         The byte enable mask for the write
//! @param wdata        The data to be written (matching the mask)

void
Or1ksimSC::staticWriteUpcall( void              *instancePtr,
			      unsigned long int  addr,
			      unsigned long int  mask,
			      unsigned long int  wdata )
{
  Or1ksimSC *classPtr = (Or1ksimSC *)instancePtr;

  classPtr->writeUpcall( (sc_dt::uint64)addr, (uint32_t)mask, (uint32_t)wdata );

}	// staticWriteUpcall()


//! Member function to handle read upcall from the underlying Or1ksim ISS

//! This is the entry point used by ::staticReadUpcall(). Constructs a Generic
//! transactional payload for the read, then passes it to ::doTrans() (also
//! used by ::writeUpcall()) for transport to the target.

//! @param addr  The address for the read
//! @param mask  The byte enable mask for the read

//! @return  The value read

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

  // Transport. Then return the result

  doTrans( trans );
  return  rdata;

}	// readUpcall()


//! Member function to handle write upcall from the underlying Or1ksim ISS

//! This is the entry point used by ::staticWriteUpcall(). Constructs a
//! Generic transactional payload for the write, then passes it to ::doTrans()
//! (also used by ::readUpcall()) for transport to the target.

//! @param addr   The address for the write
//! @param mask   The byte enable mask for the write
//! @param wdata  The data to be written (matching the mask)

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

  // Transport.

  doTrans( trans );

}	// writeUpcall()


//! TLM transport to the target

//! Calls the blocking transport routine for the initiator socket (@see
//! ::dataBus).

//! @param trans  The transaction payload

void
Or1ksimSC::doTrans( tlm::tlm_generic_payload &trans )
{
  sc_core::sc_time  dummyDelay = sc_core::SC_ZERO_TIME;

  // Call the transport

  dataBus->b_transport( trans, dummyDelay );

}	// doTrans()


// EOF
