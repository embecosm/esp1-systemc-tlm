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

// Implementation of 16450 UART SystemC module.

// $Id$


#include <iostream>
#include <iomanip>

#include "UartSC.h"


SC_HAS_PROCESS( UartSC );

//! Custom constructor for the UART module

//! Passes the name to the parent constructor. Sets the model endianism
//! (UartSC::isLittleEndian).

//! Sets up threads listening to the bus (UartSC::busThread()) and the Rx pin
//! (UartSC::rxThread()).

//! Registers UartSC::busReadWrite() as the callback for blocking transport on
//! the UartSC::bus socket.

//! Zeros the registers, but leaves the UartSC::divLatch unset - it is
//! undefined until used.

//! @param name             The SystemC module name, passed to the parent
//!                         constructor
//! @param _isLittleEndian  The model endianism

UartSC::UartSC( sc_core::sc_module_name  name,
		bool                     _isLittleEndian ) :
  sc_module( name ),
  isLittleEndian( _isLittleEndian ),
  intrPending( 0 )
{
  // Set up the thread for the bus side, method for terminal side
  // (statically sensitive to Rx)

  SC_THREAD( busThread );

  SC_METHOD( rxMethod );
  sensitive << rx;
  dont_initialize();

  // Register the blocking transport method

  bus.register_b_transport( this, &UartSC::busReadWrite );

  // Initialize the Uart. Clear regs. Other internal state (divLatch) is
  // undefined until set.

  bzero( (void *)&regs, sizeof( regs ));

}	/* UartSC() */


//! SystemC thread listening for transmit traffic on the bus

//! Sits in a loop. Initially sets the line status register to indicate the
//! buffer is empty (reset will clear these bits) and sends an interrupt (if
//! enabled) to indicate the buffer is empty.

//! Then waits for the UartSC::txReceived event to be triggered (this happens
//! when new data is written into the transmit buffer by a bus write).

//! On receipt of a character writes the char onto the Tx FIFO.

void
UartSC::busThread()
{
  // Loop listening for changes on the Tx buffer, waiting for a baud rate
  // delay then sending to the terminal

  while( true ) {

    set( regs.lsr, UART_LSR_THRE );	// Indicate buffer empty
    set( regs.lsr, UART_LSR_TEMT );
    genIntr( UART_IER_TBEI );		// Interrupt if enabled

    wait( txReceived );			// Wait for a Tx request

    tx.write( regs.thr );		// Send char to terminal
  }
}	// busThread()


//! SystemC method sensitive to data on the Rx buffer

//! Copies the character received into the read buffer.

//! Sets the data ready flag of the line status register and sends an
//! interrupt (if enabled) to indicate the data is ready.

//! @note The terminal attached to the FIFO is responsible for modeling any
//!       wire delay on the Rx.

void
UartSC::rxMethod()
{
  regs.rbr  = rx.read();

  sc_core::sc_time  now = sc_core::sc_time_stamp();
  printf( "Char read at    %12.9f sec\n", now.to_seconds());

  set( regs.lsr, UART_LSR_DR );		// Mark data ready
  genIntr( UART_IER_RBFI );		// Interrupt if enabled

}	// rxMethod()


//! TLM2.0 blocking transport routine for the UART bus socket

//! Receives transport requests on the target socket.

//! Break out the address, data and byte enable mask. Use the byte enable mask
//! to identify the byte address. Allow for model endianism in calculating
//! this (see UartSC::isLittleEndian).

//! Switches on the command and calls UartSC::busRead() or UartSC::buswrite()
//! routines to do the behavior

//! Increases the delay as appropriate and sets a success response.

//! @param payload    The transaction payload
//! @param delayTime  How far the initiator is beyond baseline SystemC
//!                   time. For use with temporal decoupling.

void
UartSC::busReadWrite( tlm::tlm_generic_payload &payload,
		      sc_core::sc_time         &delay )
{
  // Break out the address, mask and data pointer. This should be only a
  // single byte access.

  sc_dt::uint64      addr    = payload.get_address();
  unsigned char     *maskPtr = payload.get_byte_enable_ptr();
  unsigned char     *dataPtr = payload.get_data_ptr();

  int                offset;		// Data byte offset in word
  unsigned char      uaddr;		// UART address

  // Deduce the byte address, allowing for endianism of the ISS

  switch( *((uint32_t *)maskPtr) ) {
  case 0x000000ff: offset = isLittleEndian ? 0 : 3; break;
  case 0x0000ff00: offset = isLittleEndian ? 1 : 2; break;
  case 0x00ff0000: offset = isLittleEndian ? 2 : 1; break;
  case 0xff000000: offset = isLittleEndian ? 3 : 0; break;

  default:		// Invalid request

    payload.set_response_status( tlm::TLM_GENERIC_ERROR_RESPONSE );
    return;
  }

  // Mask off the address to its range. This ought to have been done already
  // by an arbiter/decoder.

  uaddr = (unsigned char)((addr + offset) & UART_ADDR_MASK);

  // Which command?

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:

    dataPtr[offset] = busRead( uaddr );
    break;

  case tlm::TLM_WRITE_COMMAND:

    busWrite( uaddr, dataPtr[offset] );
    break;

  case tlm::TLM_IGNORE_COMMAND:

    payload.set_response_status( tlm::TLM_GENERIC_ERROR_RESPONSE );
    return;
  }

  // Single byte accesses always work

  payload.set_response_status( tlm::TLM_OK_RESPONSE );

}	// busReadWrite()


//! Process a read on the UART bus

//! Switch on the address to determine behavior
//! - UART_BUF
//!   - if UART_LSR_DLAB is set, read the low byte of the clock divisor
//!   - otherwise get the value from the read buffer and
//!     clear the data ready flag in the line status register
//! - UART_IER
//!   - if UART_LSR_DLAB is set, read the high byte of the clock divisor
//!   - otherwise get the instruction enable register
//! - UART_IIR Get the interrupt indicator register and
//!   clear the most important pending interrupt (UartSC::clrIntr())
//! - UART_LCR Get the line control register
//! - UART_MCR Ignored - write only register
//! - UART_LSR Get the line status register
//! - UART_MSR Get the modem status registe
//! - UART_SCR Get the scratch register

//! @param uaddr  The address of the register being accessed

//! @return  The value read

unsigned char
UartSC::busRead( unsigned char  uaddr )
{
  unsigned char  res;		// The result to return

  // State machine lookup on the register

  switch( uaddr ) {

  case UART_BUF:

    if( isSet(regs.lcr, UART_LCR_DLAB ) ) {	// DLL byte
      res = (unsigned char)(divLatch & 0x00ff);
    }
    else {
      res = regs.rbr;				// Get the read data
      clr( regs.lsr, UART_LSR_DR );		// Clear the data ready bit
      clrIntr( UART_IER_RBFI );
    }

    break;

  case UART_IER:

    if( isSet( regs.lcr, UART_LCR_DLAB ) ) {	// DLH byte
      res = (unsigned char)((divLatch & 0xff00) >> 8);
    }
    else {
      res = regs.ier;
    }

    break;

  case UART_IIR:

    res = regs.iir;
    clrIntr( UART_IER_TBEI );
    break;

  case UART_LCR: res = regs.lcr; break;
  case UART_MCR: res = 0;        break;		// Write only
  case UART_LSR:

    res = regs.lsr;
    clr( regs.lsr, UART_LSR_BI );
    clr( regs.lsr, UART_LSR_FE );
    clr( regs.lsr, UART_LSR_PE );
    clr( regs.lsr, UART_LSR_OE );
    clrIntr( UART_IER_RLSI );
    break;

  case UART_MSR:

    res      = regs.msr;
    regs.msr = 0;
    clrIntr( UART_IER_MSI );
    modemLoopback();				// May need resetting
    break;

  case UART_SCR: res = regs.scr; break;
  }

  return res;

}	// busRead()


//! Process a write on the UART bus

//! Switch on the address to determine behavior
//! - UART_BUF
//!   - if UART_LSR_DLAB is set, write the low byte of the clock divisor and
//!     recalculate the character delay (UartSC::resetCharDelay())
//!   - otherwise write the data to the transmit buffer, clear the buffer
//!     empty flags and notify the UartSC::busThread() using the
//!     UartSC::txReceived SystemC event.
//! - UART_IER
//!   - if UART_LSR_DLAB is set, write the high byte of the clock divisor and
//!     recalculate the character delay (UartSC::resetCharDelay())
//!   - otherwise set the instruction enable register
//! - UART_IIR Ignored - read only
//! - UART_LCR Set the line control register
//! - UART_MCR Set the modem control regsiter
//!   - if loopback is set, set the MSR registers to correspond
//! - UART_LSR Ignored - read only
//! - UART_MSR Ignored - read only
//! - UART_SCR Set the scratch register

//! @param uaddr  The address of the register being accessed
//! @param wdata  The value to be written

void
UartSC::busWrite( unsigned char  uaddr,
		  unsigned char  wdata )
{
  // State machine lookup on the register

  switch( uaddr ) {

  case UART_BUF:

    if( isSet( regs.lcr, UART_LCR_DLAB ) ) {	// DLL
      divLatch = (divLatch & 0xff00) | (unsigned short int)wdata;
    }
    else {
      regs.thr = wdata;

      clr( regs.lsr, UART_LSR_TEMT );		// Tx buffer now full
      clr( regs.lsr, UART_LSR_THRE );
      clrIntr( UART_IER_TBEI );

      txReceived.notify();			// Tell the bus thread
    }

    break;

  case UART_IER:

    if( isSet( regs.lcr, UART_LCR_DLAB ) ) {	// DLH
      divLatch = (divLatch & 0x00ff) | ((unsigned short int)wdata << 8);
    }
    else {
      regs.ier = wdata;
    }

    break;

  case UART_IIR:                   break;	// Read only
  case UART_LCR: regs.lcr = wdata; break;
  case UART_MCR: 

    regs.mcr = wdata;
    modemLoopback();
    break;

  case UART_LSR:                   break;	// Read only
  case UART_MSR:                   break;	// Read only
  case UART_SCR: regs.scr = wdata; break;
  }

}	// busWrite()


//! Generate modem loopback signals

//! Software relies on this to detect the UART type. Set the modem status bits
//! as defined for modem loopback

void
UartSC::modemLoopback()
{
  // Only if we are in loopback state

  if( isClr( regs.mcr, UART_MCR_LOOP )) {
    return;
  }

  // Delta status bits for what is about to change.

  if( (isSet( regs.mcr, UART_MCR_RTS ) && isClr( regs.msr, UART_MSR_CTS )) ||
      (isClr( regs.mcr, UART_MCR_RTS ) && isSet( regs.msr, UART_MSR_CTS )) ) {
    set( regs.msr, UART_MSR_DCTS );
  }
  else {
    clr( regs.msr, UART_MSR_DCTS );
  }

  if( (isSet( regs.mcr, UART_MCR_DTR ) && isClr( regs.msr, UART_MSR_DSR )) ||
      (isClr( regs.mcr, UART_MCR_DTR ) && isSet( regs.msr, UART_MSR_DSR )) ) {
    set( regs.msr, UART_MSR_DDSR );
  }
  else {
    clr( regs.msr, UART_MSR_DDSR );
  }

  if( (isSet( regs.mcr, UART_MCR_OUT1 ) && isClr( regs.msr, UART_MSR_RI )) ||
      (isClr( regs.mcr, UART_MCR_OUT1 ) && isSet( regs.msr, UART_MSR_RI )) ) {
    set( regs.msr, UART_MSR_TERI );
  }
  else {
    clr( regs.msr, UART_MSR_TERI );
  }

  if( (isSet( regs.mcr, UART_MCR_OUT2 ) && isClr( regs.msr, UART_MSR_DCD )) ||
      (isClr( regs.mcr, UART_MCR_OUT2 ) && isSet( regs.msr, UART_MSR_DCD )) ) {
    set( regs.msr, UART_MSR_DDCD );
  }
  else {
    clr( regs.msr, UART_MSR_DDCD );
  }

  // Loopback status bits

  if( isSet( regs.mcr, UART_MCR_RTS )) {	// CTS = RTS
    set( regs.msr, UART_MSR_CTS );
  }
  else {
    clr( regs.msr, UART_MSR_CTS );
  }

  if( isSet( regs.mcr, UART_MCR_DTR )) {	// DSR = DTR
    set( regs.msr, UART_MSR_DSR );
  }
  else {
    clr( regs.msr, UART_MSR_DSR );
  }

  if( isSet( regs.mcr, UART_MCR_OUT1 )) {	// RI = OUT1
    set( regs.msr, UART_MSR_RI );
  }
  else {
    clr( regs.msr, UART_MSR_RI );
  }

  if( isSet( regs.mcr, UART_MCR_OUT2 )) {	// DSR = DTR
    set( regs.msr, UART_MSR_DCD );
  }
  else {
    clr( regs.msr, UART_MSR_DCD );
  }

  if( isSet( regs.msr, UART_MSR_DCTS ) |
      isSet( regs.msr, UART_MSR_DDSR ) |
      isSet( regs.msr, UART_MSR_TERI ) |
      isSet( regs.msr, UART_MSR_DDCD ) ) {
    genIntr( UART_IER_MSI );
  }

}	// modemLoopback()


//! Internal utility to set the IIR flags

//! The IIR bits are set for the highest priority outstanding interrupt.

//! @return  True if any interrupts are pending

bool
UartSC::setIntrFlags()
{
    clr( regs.iir, UART_IIR_MASK );			// Clear current

    if( isSet( intrPending, UART_IER_RLSI )) {		// Priority order
      set( regs.iir, UART_IIR_RLS );
    }
    else if( isSet( intrPending, UART_IER_RBFI )) {
      set( regs.iir, UART_IIR_RDA );
    }
    else if( isSet( intrPending, UART_IER_TBEI )) {
      set( regs.iir, UART_IIR_THRE );
    }
    else{
      set( regs.iir, UART_IIR_MOD );
    }

    return 0 != (intrPending & UART_IER_VALID);

}	// setIntrFlags()


//! Generate an interrupt

//! If the particular interrupt is enabled, set the relevant interrupt
//! indicator flag, mark an interrupt as pending.

//! @note There is no actual interrupt port in this class. The interrupt
//!       signal driving functionality will be added in a derived class

//! @param ierFlag  Indicator of which interrupt is to be cleared (as IER bit).

void
UartSC::genIntr( unsigned char  ierFlag )
{
  if( isSet( regs.ier, ierFlag )) {
    set( intrPending, ierFlag );	// Mark this interrupt as pending.

    (void)setIntrFlags();		// Show highest priority

    clr( regs.iir, UART_IIR_IPEND );	// Mark (0 = pending) and queue
  }
}	// genIntr()


//! Clear an interrupt

//! Clear the interrupts in priority order.

//! If no interrupts remain asserted, clear the interrupt pending flag

//! @note There is no actual interrupt port in this class. The interrupt
//!       signal driving functionality will be added in a derived class

//! @param ierFlag  Indicator of which interrupt is to be cleared (as IER bit).

void
UartSC::clrIntr( unsigned char ierFlag )
{
  clr( intrPending, ierFlag );

  if( !setIntrFlags()) {		// Deassert if none left
    set( regs.iir, UART_IIR_IPEND );	// 1 = not pending
  }
}	// clrIntr()


//! Set a bits in a register

//! @param reg    The register concerned
//! @param flags  The bits to set

inline void
UartSC::set( unsigned char &reg,
	     unsigned char  flags )
{
  reg |= flags;

}	// set()


//! Clear a bits in a register

//! @param reg    The register concerned
//! @param flags  The bits to set

inline void
UartSC::clr( unsigned char &reg,
	     unsigned char  flags )
{
  reg &= ~flags;

}	// clr()


//! Report if bits are set in a register

//! @param reg    The register concerned
//! @param flags  The bit to set

//! @return  True if the bit is set

inline bool
UartSC::isSet( unsigned char  reg,
	       unsigned char  flags )
{
  return  flags == (reg & flags);

}	// isSet()


//! Report if bits are clear in a register

//! @param reg    The register concerned
//! @param flags  The bit to set

//! @return  True if the bit is clear

inline bool
UartSC::isClr( unsigned char  reg,
	       unsigned char  flags )
{
  return  flags != (reg & flags);

}	// isClr()


// EOF
