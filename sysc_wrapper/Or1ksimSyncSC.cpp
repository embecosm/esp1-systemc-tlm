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
// with SystemC time synchronization

// $Id$


#include "Or1ksimSyncSC.h"


SC_HAS_PROCESS( Or1ksimSyncSC );

//! Custom constructor for the Or1ksimSyncSC SystemC module

//! Initializes the underlying Or1ksim ISS, passing in the configuration and
//! image files, a pointer to this class instance and pointers to the static
//! upcall routines (@see ::staticReadUpcall() and ::staticWriteUpcall()).

//! Declares the main SystemC thread, ::run().

//! @param name        SystemC module name
//! @param configFile  Config file for the underlying ISS
//! @param imageFile   Binary image to run on the ISS


Or1ksimSyncSC::Or1ksimSyncSC ( sc_core::sc_module_name  name,
		       const char              *configFile,
		       const char              *imageFile ) :
  sc_module( name ),
  dataBus( "data_initiator" )
{
  or1ksim_init( configFile, imageFile, this, staticReadUpcall,
		staticWriteUpcall );

  SC_THREAD( run );		  // Thread to run the ISS

}	/* Or1ksimSyncSC() */


//! Identify the endianism of the underlying Or1ksim ISS

//! Public utility to allow other modules to identify the model endianism

//! @return  True if the Or1ksim ISS is little endian

bool
Or1ksimSyncSC::isLittleEndian()
{
  return (1 == or1ksim_is_le());

}	// or1ksimIsLe()


//! Get the clock rate of the underlying Or1ksim ISS

//! Public utility to allow other modules to identify the ISS clock rate

//! @return  The clock rate in Hz of the underlying Or1ksim ISS

unsigned long int
Or1ksimSyncSC::getClockRate()
{
  return or1ksim_clock_rate();

}	// getClockRate()


//! The SystemC thread running the underlying ISS

//! The thread calls the ISS to run forever.

//! There are upcalls for read and write to peripherals (@see
//! ::staticReadUpcall(), ::staticWriteUpcall, ::readUpcall(),
//! ::writeUpcall()), which provide opportunities for the thread to yield, and
//! so not block the simulation.

void
Or1ksimSyncSC::run()
{
  scLastUpTime   = sc_core::sc_time_stamp();
  or1kLastUpTime = or1ksim_time();

  (void)or1ksim_run( -1.0 );

}	// Or1ksimSyncSC()


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
//!                     constructor, ::Or1ksimSyncSC()).
//! @param addr         The address for the read
//! @param mask         The byte enable mask for the read

//! @return  The value read, cast back to a C type which can hold 32 bits

unsigned long int
Or1ksimSyncSC::staticReadUpcall( void              *instancePtr,
			     unsigned long int  addr,
			     unsigned long int  mask )
{
  Or1ksimSyncSC *classPtr = (Or1ksimSyncSC *)instancePtr;

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
//!                     constructor, ::Or1ksimSyncSC()).
//! @param addr         The address for the write
//! @param mask         The byte enable mask for the write
//! @param wdata        The data to be written (matching the mask)

void
Or1ksimSyncSC::staticWriteUpcall( void              *instancePtr,
			      unsigned long int  addr,
			      unsigned long int  mask,
			      unsigned long int  wdata )
{
  Or1ksimSyncSC *classPtr = (Or1ksimSyncSC *)instancePtr;

  classPtr->writeUpcall( (sc_dt::uint64)addr, (uint32_t)mask, (uint32_t)wdata );

}	// staticWriteUpcall()


//! Member function to handle read upcall from the underlying Or1ksim ISS

//! This is the entry point used by ::staticReadUpcall(). Constructs a Generic
//! transactional payload for the read, then passes it to ::syncTrans() (also
//! used by ::writeUpcall()) for timing synchronization and transport to the
//! target.

//! @param addr  The address for the read
//! @param mask  The byte enable mask for the read

//! @return  The value read

uint32_t
Or1ksimSyncSC::readUpcall( sc_dt::uint64  addr,
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


//! Member function to handle write upcall from the underlying Or1ksim ISS

//! This is the entry point used by ::staticWriteUpcall(). Constructs a Generic
//! transactional payload for the write, then passes it to ::syncTrans() (also
//! used by ::readUpcall()) for timing synchronization and transport to the
//! target.

//! @param addr   The address for the write
//! @param mask   The byte enable mask for the write
//! @param wdata  The data to be written (matching the mask)

void
Or1ksimSyncSC::writeUpcall( sc_dt::uint64  addr,
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


//! Synchronized TLM transport to the target

//! The module synchronizes with SystemC for the time consumed by the ISS,
//! then calls the blocking transport routine for the initiator socket (@see
//! ::dataBus). SystemC time is adjusted for any delay due to the external
//! entity.

//! @param trans  The transaction payload

void
Or1ksimSyncSC::syncTrans( tlm::tlm_generic_payload &trans )
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
  dataBus->b_transport( trans, delay );
  scLastUpTime += delay;

}	// syncTrans()


// EOF
