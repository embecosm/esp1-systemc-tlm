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

// Definition of 16450 UART Synchronous SystemC module.

// $Id$


#ifndef UART_SYNC_SC__H
#define UART_SYNC_SC__H

#include "UartSC.h"

#define UART_READ_NS       60	//!< Time to access the UART for read
#define UART_WRITE_NS      60	//!< Time to access the UART for write


//! SystemC module class for a 16450 UART with synchronized timing

//! Derived from the UartSC:: class. Updates the UartSC::busThread() thread and
//! UartSC::busReadWrite() blocking TLM callback to provide synchronous
//! timing.

//! The UartSC::busWrite() function is extended to update the baud rate timing
//! based on changes to the UART registers.


class UartSyncSC
  : public UartSC
{
public:

  UartSyncSC( sc_core::sc_module_name  name,
	      unsigned long int        _clockRate,
	      bool                     _isLittleEndian );


protected:

  // Reimplemetned blocking transport function, which adds timing, further
  // reimplmented in derived classes. New version of the busWrite function, to
  // update the character delay if relevant registers are update will not be
  // reimplemented further.

  virtual void  busReadWrite( tlm::tlm_generic_payload &payload,
			      sc_core::sc_time         &delay );

          void  busWrite( unsigned char  uaddr,
			  unsigned char  wdata );


private:

  // Reimplemented bus thread, which adds a timing delay before passing on hte
  // character read. Not reimplemented again.

  void          busThread();

  // Function to work out the baud rate character delay. Won't be reused in
  // derived classes.

  void  resetCharDelay();

  // Additional status info for this class only

  unsigned long int   clockRate;	//!< External clock into UART
  sc_core::sc_time    charDelay;	//!< Total time to Tx a char

};	// UartSyncSC()


#endif	// UART_SYNC_SC__H


// EOF
