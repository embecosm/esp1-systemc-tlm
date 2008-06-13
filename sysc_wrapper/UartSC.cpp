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

// Implementation of 16450 UART SystemC object.

// $Id$


#include <iostream>

#include "UartSC.h"
#include "Or1ksimSC.h"


// Constructor

SC_HAS_PROCESS( UartSC );

UartSC::UartSC( sc_core::sc_module_name  name ) :
  sc_module( name )
{
  // Register the blocking transport method

  uartInPort.register_b_transport( this, &UartSC::doReadWrite );

  // Reset the UART and record whether the ISS is little endian

  uartInit();
  isLittleEndian = Or1ksimSC::isLittleEndian();

}	/* UartSC() */


// Empty destructor

UartSC::~UartSC()
{
}	// ~UartSC()
  

// The blocking transport routine.

void
UartSC::doReadWrite( tlm::tlm_generic_payload &payload,
		      sc_core::sc_time         &delayTime )
{
  // Which command?

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:
    doRead( payload, delayTime );
    break;

  case tlm::TLM_WRITE_COMMAND:
    doWrite( payload, delayTime );
    break;

  case tlm::TLM_IGNORE_COMMAND:
    std::cerr << "UartSC: Unexpected TLM_IGNORE_COMMAND" << std::endl;
    break;
  }

}	// doReadWrite()


// Process a read

void
UartSC::doRead( tlm::tlm_generic_payload &payload,
		sc_core::sc_time         &delayTime )
{
  // Break out the address, mask and data pointer. This should be only a
  // single byte access.

  unsigned long int  addr   = (unsigned long int)payload.get_address();
  unsigned long int  offset;
  unsigned char     *mask   = payload.get_byte_enable_ptr();
  unsigned char     *dat    = payload.get_data_ptr();
  unsigned char      res;	// Result to transmit back
  int                ch;	// Char read from xterm

  for( int i = 0 ; i < 4 ; i++ ) {
    if( TLM_BYTE_ENABLED == mask[i] ) {
      offset = i;
      break;
    }
  }

  // Find the address and mask off to just the UART space.

  addr  = isLittleEndian ? addr + offset : addr + (3 - offset);
  addr &= UART_SIZE - 1;

  // State machine lookup

  switch( addr ) {

  case UART_RXBUF:

    if( (regs[UART_LCR] & UART_LCR_DLAB) != 0 ) {

      // Get the divisor latch low byte

      res = (unsigned char)(divisorLatch & 0x00ff);
    }
    else {

      // Read a byte. This should work, since we are only called when the test
      // has shown there is a byte available. However clear the data ready bit
      // when done.

      res = lastReadByte;
      regs[UART_LSR] &= ~UART_LSR_DR;	// No data
    }

    break;

  case UART_IER:

    if( (regs[UART_LCR] & UART_LCR_DLAB) != 0 ) {

      // Get the divisor latch high byte

      res = (unsigned char)((divisorLatch & 0xff00) >> 8);
    }
    else {

      // Read the interrupt enable bits. Not currently meaningful.

      std::cout << "UART interrupts not currently supported" << std::endl;
      res = regs[UART_IER];
    }

    break;

  case UART_IIR:

    // Read the interrupt ID bits. Not currently meaningful.

    std::cout << "UART interrupts not currently supported" << std::endl;

    res = regs[UART_IIR];
    break;

  case UART_LCR:

    // Read the line control status. Not hugely valuable, since it is too much
    // detail for a TLM.

    res = regs[UART_LCR];
    break;

  case UART_MCR:

    // Read the modem control register. Of these, only loopback is really
    // meaningful.

    res = regs[UART_MCR];
    break;

  case UART_LSR:

    // The interesting register. If there is currently no data available, we
    // do a read, to see if the xterm has anything, and if so, set the data
    // ready bit.

    if( 0 == (regs[UART_LSR] & UART_LSR_DR)) {

      ch = remoteRead();		// From xterm

      if( ch < 0 ) {
	regs[UART_LSR] &= ~UART_LSR_DR;	// No data
      }
      else {
	lastReadByte = (unsigned char)ch;
	regs[UART_LSR] |= UART_LSR_DR;	// New data
      }
    }

    res = regs[UART_LSR];
    break;

  case UART_MSR:

    // Read the modem status. Not hugely valuable, since it is too much detail
    // for a TLM.

    res = regs[UART_MSR];
    break;

  case UART_SCR:

    // Read the scratch regiser.

    res = regs[UART_SCR];
    break;
  }

  // Put the result in the right place in the payload and set a response. All
  // reads are successful.

  dat[offset] = (unsigned char)(res & 0xff);
  payload.set_response_status( tlm::TLM_OK_RESPONSE );

}	// doRead()


void
UartSC::doWrite( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime )
{
  // Break out the address, mask and data pointer. This should be only a
  // single byte access.

  unsigned long int  addr   = (unsigned long int)payload.get_address();
  unsigned long int  offset;
  unsigned char     *mask   = payload.get_byte_enable_ptr();
  unsigned char     *dat    = payload.get_data_ptr();

  for( int i = 0 ; i < 4 ; i++ ) {
    if( TLM_BYTE_ENABLED == mask[i] ) {
      offset = i;
      break;
    }
  }

  unsigned char  ch = dat[offset];

  // Find the address and mask off to just the UART space.

  addr  = isLittleEndian ? addr + offset : addr + (3 - offset);
  addr &= UART_SIZE - 1;

  // State machine lookup

  tlm::tlm_response_status  res = tlm::TLM_OK_RESPONSE;   // Response

  switch( addr ) {

  case UART_TXBUF:

    if( (regs[UART_LCR] & UART_LCR_DLAB) != 0 ) {

      // Set the divisor latch low byte

      divisorLatch &= 0xff00;
      divisorLatch |= (unsigned short int)ch;
    }
    else {

      // Write a byte. This always works - we immediately drive the character
      // to the terminal

      remoteWrite( ch );
    }

    break;

  case UART_IER:

    if( (regs[UART_LCR] & UART_LCR_DLAB) != 0 ) {

      // Set the divisor latch high byte

      divisorLatch &= 0x00ff;
      divisorLatch |= ((unsigned short int)ch) << 8;
    }
    else {

      // Write the interrupt enable bits. Not currently meaningful.

      std::cout << "UART interrupts not currently supported" << std::endl;
      regs[UART_IER] = ch;
    }

    break;

  case UART_IIR:

    // Not a writeable register on the 16450 (it is on the 16550). This is an
    // error.

    std::cout << "UART register " << UART_IIR << " is not writeable"
	      << std::endl;

    res = tlm::TLM_GENERIC_ERROR_RESPONSE;
    break;

  case UART_LCR:

    // Write the line control status. Not hugely valuable, since it is too much
    // detail for a TLM.

    regs[UART_LCR] = ch;
    break;

  case UART_MCR:

    // Write the modem control register. Of these, only loopback is really
    // meaningful, but not currently supported.

    regs[UART_MCR] = ch;
    break;

  case UART_LSR:

    // The interesting register when reading, but not so meaningful when
    // writing. If the DR bit is set by writing, a character could be dropped.

    regs[UART_LSR] = ch;
    break;

  case UART_MSR:

    // Write the modem status. Not hugely valuable, since it is too much detail
    // for a TLM.

    regs[UART_MSR] = ch;
    break;

  case UART_SCR:

    // Write the scratch regiser.

    regs[UART_SCR];
    break;
  }

  payload.set_response_status( res );	// Set a response.

}	// doWrite()


// Initialize the Uart. This just means clearing all the registers to zero.

void
UartSC::uartInit()
{
  bzero( (void *)regs, UART_SIZE );

}	// uartInit()


// Do a remote read from the terminal. -1 if there is no char available.

int
UartSC::remoteRead()
{
  tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload();

  // Set up the command. The address is irrelevant for the xterm

  trans->set_read();

  // Set up the size (always 4 bytes) and allocate a suitable data pointer

  unsigned char *dataPtr = new unsigned char[4];

  trans->set_data_length( 4 );
  trans->set_data_ptr( dataPtr );

  // Set up the byte enable mask (always 4 bytes). This always uses byte 0

  unsigned char *byteMask = new unsigned char[4];

  bzero( byteMask, 4 );
  byteMask[0] = TLM_BYTE_ENABLED;

  trans->set_byte_enable_length( 4 );
  trans->set_byte_enable_ptr( byteMask );

  // Send it

  sc_core::sc_time  t = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  uartOutPort->b_transport( *trans, t );

  // The result should be in the data. Copy it for return.

  int                       ch  = (int)dataPtr[0];
  tlm::tlm_response_status  res = trans->get_response_status();

  // Free up the memory

  delete [] byteMask;
  delete [] dataPtr;
  delete trans;

  return  (tlm::TLM_OK_RESPONSE == res) ? ch : -1;

}	// remoteRead()


// Write to the remove xterm. Assumed to succeed

void
UartSC::remoteWrite( unsigned char  ch )
{
  tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload();

  // Set up the command. The address is irrelevant for the xterm

  trans->set_write();

  // Set up the size (always 4 bytes) and allocate a suitable data pointer

  unsigned char *dataPtr = new unsigned char[4];

  bzero( dataPtr, 4 );
  dataPtr[0] = ch;

  trans->set_data_length( 4 );
  trans->set_data_ptr( dataPtr );

  // Set up the byte enable mask (always 4 bytes)

  unsigned char *byteMask = new unsigned char[4];

  bzero( byteMask, 4 );
  byteMask[0] = TLM_BYTE_ENABLED;

  trans->set_byte_enable_length( 4 );
  trans->set_byte_enable_ptr( byteMask );

  // Send it. We assume it just worked (for now)

  sc_core::sc_time  t = sc_core::sc_time( 0.0, sc_core::SC_SEC );
  uartOutPort->b_transport( *trans, t );

  // Free up the memory

  delete [] byteMask;
  delete [] dataPtr;
  delete trans;

}	// remoteWrite()


// EOF
