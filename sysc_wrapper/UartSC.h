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

// Definition of 16450 UART SystemC module.

// $Id$


#ifndef UART_SC__H
#define UART_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"


#define UART_ADDR_MASK      7	//!< Mask for addresses (3 bit bus)

// Offsets for the 16450 UART registers

#define UART_BUF  0		//!< R/W: Rx/Tx buffer, DLAB=0
#define UART_IER  1 		//!< R/W: Interrupt Enable Register, DLAB=0
#define UART_IIR  2 		//!< R: Interrupt ID Register
#define UART_LCR  3 		//!< R/W: Line Control Register
#define UART_MCR  4 		//!< W: Modem Control Register
#define UART_LSR  5 		//!< R: Line Status Register
#define UART_MSR  6 		//!< R: Modem Status Register
#define UART_SCR  7 		//!< R/W: Scratch Register

// Interrupt Enable register bits of interest

#define UART_IER_ETBEI  0x02  	//!< Enable transmitter holding register int.
#define UART_IER_ERBFI  0x01  	//!< Enable receiver data interrupt

// Interrupt Identification register bits and interrupt masks of interest

#define UART_IIR_RDI    0x04  	//!< Receiver data interrupt
#define UART_IIR_THRE   0x02  	//!< Transmitter holding reg empty interrupt
#define UART_IIR_IPEND  0x00  	//!< Interrupt pending

// Line Control register bits of interest and data word length mask

#define UART_LCR_DLAB   0x80	//!< Divisor latch access bit
#define UART_LCR_PEN    0x08  	//!< Parity Enable
#define UART_LCR_STB    0x04  	//!< Stop bits: 0=1 stop bit, 1= 2 stop bits
#define UART_LCR_MASK   0x03	//!< 2-bit mask for word length

// Line Status register bits of interest

#define UART_LSR_TEMT   0x40  	//!< Transmitter serial register empty
#define UART_LSR_THRE   0x20  	//!< Transmitter holding register empty
#define UART_LSR_DR     0x01  	//!< Receiver data ready


//! SystemC module class for a 16450 UART.

//! Provides a TLM 2.0 simple target convenience socket for access to the UART
//! regsters, unsigned char SystemC FIFO ports (to a 1 byte FIFO) for the Rx
//! and Tx pins and a bool SystemC signal for the interrupt pin.

//! Two threads are provided, one waiting for transmit requests from the bus,
//! the other waiting for data on the Rx pin.

class UartSC
: public sc_core::sc_module
{
 public:

  // Constructor

  UartSC( sc_core::sc_module_name  name,
	  bool                     _isLittleEndian );

  //! Simple target convenience socket for UART bus access to registers
  
  tlm_utils::simple_target_socket<UartSC>     bus;

  // Fifos for the terminal to read/write to us

  sc_core::sc_fifo_in<unsigned char>          rx;	//!< FIFO in from Rx
  sc_core::sc_fifo_out<unsigned char>         tx;	//!< FIFO out to Tx

  // Signal for the interrupt pin

  sc_core::sc_out<bool>                       intr;	//!< Interrupt output


 protected:

  // The two threads running the behavior of the UART. These will be replaced
  // by later derived classes, so are declared virtual.

  virtual void  busThread();
  virtual void  rxThread();

  // Blocking transport function. Split out separate read and write
  // functions. The busReadWrite() and busWrite() functions will be replaced
  // by later derived classes, so are declared virtual. The busRead()
  // function will be used by later derived classes, but not replaced.

  virtual void   busReadWrite( tlm::tlm_generic_payload &payload,
			       sc_core::sc_time         &delay );

  unsigned char  busRead( unsigned char  uaddr );

  virtual void   busWrite( unsigned char  uaddr,
			   unsigned char  wdata );

  // Utility routines for interrupt handling. Reused in later derived classes.

  void  genInt( unsigned char  iflag );
  void  clearInt();

  // Flag handling utilities. Also reused in later derived classes.

  void  set( unsigned char &reg,
	     unsigned char  flag );
  void  clr( unsigned char &reg,
	     unsigned char  flag );
  bool  isSet( unsigned char  reg,
	       unsigned char  flag );
  bool  isClr( unsigned char  reg,
	       unsigned char  flag );

  //! UART event triggered by the CPU writing into the Tx buffer. This is
  //! reused by later derived classes.

  sc_core::sc_event  txReceived;

  //! UART registers. These will be reused in later derived classes
  //! - rbr: R: Rx buffer,		      
  //! - thr: R: Tx hold reg,		      
  //! - ier: R/W: Interrupt Enable Register   
  //! - iir: R: Interrupt ID Register	      
  //! - lcr: R/W: Line Control Register	      
  //! - mcr: W: Modem Control Register	      
  //! - lsr: R: Line Status Register	      
  //! - msr: R: Modem Status Register	      
  //! - scr: R/W: Scratch Register            

  struct {
    unsigned char  rbr;		// R: Rx buffer,
    unsigned char  thr;		// R: Tx hold reg,
    unsigned char  ier;		// R/W: Interrupt Enable Register
    unsigned char  iir;		// R: Interrupt ID Register
    unsigned char  lcr;		// R/W: Line Control Register
    unsigned char  mcr;		// W: Modem Control Register
    unsigned char  lsr;		// R: Line Status Register
    unsigned char  msr;		// R: Modem Status Register
    unsigned char  scr;		// R/W: Scratch Register            
  } regs;

  // The divisor latch is really an extra 16 bit register. It will be reused
  // in later derived classes.

  unsigned short int  divLatch;		//!< Divisor for ext clock


private:

  // Endianism is only used by routines in this class that will not be
  // replaced in later derived subclasses, to can remain private.

  bool                isLittleEndian;	//!< Is ISS little endian?

};	// UartSC()


#endif	// UART_SC__H


// EOF
