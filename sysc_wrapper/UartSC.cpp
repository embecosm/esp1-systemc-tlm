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
#include <iomanip>
#include <stdio.h>

#include "UartSC.h"
#include "Or1ksimSC.h"


// Constructor

SC_HAS_PROCESS( UartSC );

UartSC::UartSC( sc_core::sc_module_name  name,
		unsigned long int        clockRate ) :
  sc_module( name )
{
  // Set up the two threads

  SC_THREAD( uartCpuThread );
  SC_THREAD( uartTermThread );

  // Register the blocking transport method

  uartPort.register_b_transport( this, &UartSC::cpuReadWrite );

  // Reset the UART and record whether the ISS is little endian

  uartInit( clockRate );

}	/* UartSC() */


// Empty destructor

UartSC::~UartSC()
{
}	// ~UartSC()
  

// The thread listening for transmit traffic from the CPU

void
UartSC::uartCpuThread()
{
  // Loop listening for changes on the Tx buffer, waiting for a baud rate
  // delay then sending to the terminal

  while( true ) {

    wait( txReceived );			// Wait for a Tx to be requested
    wait( iState.charDelay );		// Wait baud delay

    uartTx.write( regs.thr );		// Send char to terminal

    regs.lsr |= UART_LSR_TEMT;		// Indicate buffer is now empty
    regs.lsr |= UART_LSR_THRE;
  }
}	// uartCpuThread()


// The thread listening for data on the Rx port from the terminal

void
UartSC::uartTermThread()
{
  // Loop woken up when a character is written into the fifo from the terminal.

  while( true ) {
    regs.rbr  = uartRx.read();		// Blocking read of the data
    regs.lsr |= UART_LSR_DR;		// Mark data ready
  }
}	// uartTermThread()


// Initialize the Uart.

void
UartSC::uartInit( unsigned long int  clock_rate )
{
  // Clear visible and internal state

  bzero( (void *)&regs,   sizeof( regs ));
  bzero( (void *)&iState, sizeof( iState ));

  // Set start up values

  iState.clockRate = clock_rate;

  // Note endianism

  isLittleEndian = Or1ksimSC::isLittleEndian();

}	// uartInit()


// The blocking transport routine for the CPU facing socket

void
UartSC::cpuReadWrite( tlm::tlm_generic_payload &payload,
		      sc_core::sc_time         &delayTime )
{
  // Which command?

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:
    cpuRead( payload, delayTime );
    break;

  case tlm::TLM_WRITE_COMMAND:
    cpuWrite( payload, delayTime );
    break;

  case tlm::TLM_IGNORE_COMMAND:
    std::cerr << "UartSC: Unexpected TLM_IGNORE_COMMAND" << std::endl;
    break;
  }

}	// cpuReadWrite()


// Process a read

void
UartSC::cpuRead( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime )
{
  // Break out the address, mask and data pointer. This should be only a
  // single byte access.

  sc_dt::uint64      addr    = payload.get_address();
  unsigned char     *maskPtr = payload.get_byte_enable_ptr();
  unsigned char     *dataPtr = payload.get_data_ptr();

  int                offset;

  // Deduce the byte address, allowing for endianism of the ISS

  switch( *((uint32_t *)maskPtr) ) {
  case 0x000000ff: offset = isLittleEndian ? 0 : 3; break;
  case 0x0000ff00: offset = isLittleEndian ? 1 : 2; break;
  case 0x00ff0000: offset = isLittleEndian ? 2 : 1; break;
  case 0xff000000: offset = isLittleEndian ? 3 : 0; break;

  default:		// Invalid request

    std::cerr << "Uart: cpuRead: multiple byte read - ignored\n" << std::endl;
    payload.set_response_status( tlm::TLM_GENERIC_ERROR_RESPONSE );
    return;
  }

  // Mask off the address to its range. This ought probably to have been done
  // already.

  addr = (addr + offset) & (UART_SIZE - 1);

  // State machine lookup on the register

  unsigned char      res;	// Result to transmit back

  switch( addr ) {

  case UART_BUF:	// DLL/RBR

    if( UART_LCR_DLAB == (regs.lcr & UART_LCR_DLAB) ) {
      res = (unsigned char)(iState.divLatch & 0x00ff);	// DLL byte
    }
    else {
      res       = regs.rbr;		// Get the read data
      regs.lsr &= ~UART_LSR_DR;		// Clear the data ready bit
    }

    break;

  case UART_IER:	// DLH/IER

    if( UART_LCR_DLAB == (regs.lcr & UART_LCR_DLAB) ) {
      res = (unsigned char)((iState.divLatch & 0xff00) >> 8);  // DLH byte
    }
    else {
      res = regs.ier;
    }

    break;

  case UART_IIR: res = regs.iir; break;
  case UART_LCR: res = regs.lcr; break;
  case UART_MCR: res = regs.mcr; break;
  case UART_LSR: res = regs.lsr; break;
  case UART_MSR: res = 0;        break;		// Write only
  case UART_SCR: res = regs.scr; break;
  }

  // Put the result in the right place, allowing for endianism of the ISS. All
  // reads succeed, even if they are to write only registers!

  dataPtr[offset] = res;
  payload.set_response_status( tlm::TLM_OK_RESPONSE );

}	// cpuRead()


void
UartSC::cpuWrite( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime )
{
  // Break out the address, mask and data pointer. This should be only a
  // single byte access.

  sc_dt::uint64      addr    = payload.get_address();
  unsigned char     *maskPtr = payload.get_byte_enable_ptr();
  unsigned char     *dataPtr = payload.get_data_ptr();

  int                offset;

  // Deduce the byte address, allowing for endianism of the ISS

  switch( *((uint32_t *)maskPtr) ) {
  case 0x000000ff: offset = isLittleEndian ? 0 : 3; break;
  case 0x0000ff00: offset = isLittleEndian ? 1 : 2; break;
  case 0x00ff0000: offset = isLittleEndian ? 2 : 1; break;
  case 0xff000000: offset = isLittleEndian ? 3 : 0; break;

  default:		// Invalid request

    std::cerr << "Uart: cpuWrite: multiple byte write - ignored\n" << std::endl;
    payload.set_response_status( tlm::TLM_GENERIC_ERROR_RESPONSE );
    return;
  }

  // Get the data to write. Mask off the address to its range. This ought
  // probably to have been done already.

  unsigned char ch   = dataPtr[offset];
  addr               = (addr + offset) & UART_SIZE - 1;

  // State machine lookup on the register

  switch( addr ) {

  case UART_BUF:	// DLL/THR

    if( UART_LCR_DLAB == (regs.lcr & UART_LCR_DLAB) ) {
      iState.divLatch =
	(iState.divLatch & 0xff00) | (unsigned short int)ch;
      uartResetCharDelay();
    }
    else {
      regs.thr = ch;
      regs.lsr &= ~UART_LSR_TEMT;	// Tx buffer now full
      regs.lsr &= ~UART_LSR_THRE;

      txReceived.notify();		// Tell the CPU thread
    }

    break;

  case UART_IER:	// DLH/IER

    if( UART_LCR_DLAB == (regs.lcr & UART_LCR_DLAB) ) {
      iState.divLatch =
	(iState.divLatch & 0x00ff) | ((unsigned short int)ch << 8);
      uartResetCharDelay();
    }
    else {
      regs.ier = ch;
    }

    break;

  case UART_IIR:                break;	// Read only
  case UART_LCR: regs.lcr = ch; break;
  case UART_MCR: regs.mcr = ch; break;
  case UART_LSR:                break;	// Read only
  case UART_MSR:                break;  // Read only
  case UART_SCR: regs.scr = ch; break;
  }

  payload.set_response_status( tlm::TLM_OK_RESPONSE );	// Set a response.

}	// cpuWrite()


// Recalculate charDelay after a change to the divisor latch

void
UartSC::uartResetCharDelay()
{
  iState.charDelay = sc_core::sc_time( 1.0 * (double)iState.divLatch /
				       (double)iState.clockRate,
				       sc_core::SC_SEC );

}	// uartResetCharDelay()

// EOF
