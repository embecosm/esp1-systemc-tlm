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

// Definition of the basic SystemC wrapper for Or1ksim


// $Id$


#ifndef OR1KSIM_SC__H
#define OR1KSIM_SC__H

#include <stdint.h>

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "or1ksim.h"


//! SystemC module class wrapping Or1ksim ISS

//! Provides a single thread (::run) which runs the underlying Or1ksim ISS.

class Or1ksimSC
  : public sc_core::sc_module
{
public:

  Or1ksimSC( sc_core::sc_module_name  name,
	     const char              *configFile,
	     const char              *imageFile );

  //! Initiator port for data accesses

  tlm_utils::simple_initiator_socket<Or1ksimSC>  dataBus;


protected:

  // Thread which will run the model. This will be reimplemented in later
  // derived classes to deal with timing.

  virtual void  run();

  // The common thread to make the transport calls. This will be reimplemented
  // in later derived classes to deal with timing.

  virtual void  doTrans( tlm::tlm_generic_payload &trans );


private:

  // I/O upcalls from Or1ksim, with a common synchronized transport
  // utility. These are not changed in later derived classes.

  static unsigned long int  staticReadUpcall( void              *instancePtr,
					      unsigned long int  addr,
					      unsigned long int  mask );

  static void               staticWriteUpcall( void              *instancePtr,
					       unsigned long int  addr,
					       unsigned long int  mask,
					       unsigned long int  wdata );

  uint32_t                  readUpcall( sc_dt::uint64  addr,
					uint32_t       mask );

  void                      writeUpcall( sc_dt::uint64  addr,
					 uint32_t       mask,
					 uint32_t       wdata );

};	/* Or1ksimSC() */


#endif	// OR1KSIM_SC__H


// EOF
