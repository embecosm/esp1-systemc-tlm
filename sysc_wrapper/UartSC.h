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

// Definition of 16450 UART SystemC object.

// $Id$


#ifndef UART_SC__H
#define UART_SC__H

#include "tlm.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/simple_initiator_socket.h"

// Offsets for the 16450 UART registers

#define UART_RXBUF  0		// R: Rx buffer, DLAB=0
#define UART_TXBUF  0		// W: Tx buffer, DLAB=0
#define UART_DLL  0		// R/W: Divisor Latch Low, DLAB=1
#define UART_DLH  1 		// R/W: Divisor Latch High, DLAB=1
#define UART_IER  1 		// R/W: Interrupt Enable Register
#define UART_IIR  2 		// R: Interrupt ID Register
#define UART_LCR  3 		// R/W: Line Control Register
#define UART_MCR  4 		// W: Modem Control Register
#define UART_LSR  5 		// R: Line Status Register
#define UART_MSR  6 		// R: Modem Status Register
#define UART_SCR  7 		// R/W: Scratch Register
#define UART_SIZE 8		// Size of the UART register bank

// Bit masks for valid 16450 bits in regs

#define UART_VALID_IER  0x0f
#define UART_VALID_IIR  0x07
#define UART_VALID_LCR  0xff
#define UART_VALID_MCR  0x1f
#define UART_VALID_LSR  0x7f
#define UART_VALID_MSR  0xff

// Interrupt Enable register bits

#define UART_IER_EDSSI  0x08  	// Enable modem status interrupt
#define UART_IER_ELSI   0x04  	// Enable receiver line status interrupt
#define UART_IER_ETBEI  0x02  	// Enable transmitter holding register int.
#define UART_IER_ERBFI  0x01  	// Enable receiver data interrupt

// Interrupt Identification register bits and interrupt masks

#define UART_IIR_IID1   0x02	// Interrupt ID bit #1
#define UART_IIR_IID0   0x01	// Interrupt ID bit #0
#define UART_IIR_IPEND  0x00  	// Interrupt pending

#define UART_IIR_MASK   0x06  	// Mask for the interrupt ID
#define UART_IIR_ERRI   0x06  	// Error received interrupt
#define UART_IIR_RDI    0x04  	// Receiver data interrupt
#define UART_IIR_THRE   0x02  	// Transmitter holding register empty interrupt

// Line Control register bits and word lengthmasks

#define UART_LCR_DLAB   0x80	// Divisor latch access bit
#define UART_LCR_SBC    0x40  	// Set break control
#define UART_LCR_SPAR   0x20  	// Stick parity
#define UART_LCR_EPS    0x10  	// Even parity select
#define UART_LCR_PEN    0x08  	// Parity Enable
#define UART_LCR_STB    0x04  	// Stop bits: 0=1 stop bit, 1= 2 stop bits
#define UART_LCR_WLSB1  0x02  	// Word length select bit #1
#define UART_LCR_WLSB0  0x01  	// Word length select bit #0

#define UART_LCD_MASK   0x03	// Mask for word length
#define UART_LCR_WLEN5  0x00  	// Wordlength: 5 bits
#define UART_LCR_WLEN6  0x01  	// Wordlength: 6 bits
#define UART_LCR_WLEN7  0x02  	// Wordlength: 7 bits
#define UART_LCR_WLEN8  0x03  	// Wordlength: 8 bits

// Modem Control register bits

#define UART_MCR_LBE    0x10  	// Loopback enable
#define UART_MCR_OUT2   0x08  	// Auxilary output 2 
#define UART_MCR_OUT1   0x04  	// Auxilary output 1
#define UART_MCR_RTS    0x02  	// Force RTS
#define UART_MCR_DTR    0x01  	// Force DTR

// Line Status register bits

#define UART_LSR_TEMT   0x40  	// Transmitter serial register empty
#define UART_LSR_THRE   0x20  	// Transmitter holding register empty
#define UART_LSR_BI     0x10  	// Break interrupt indicator
#define UART_LSR_FE     0x08  	// Frame error indicator
#define UART_LSR_PE     0x04  	// Parity error indicator
#define UART_LSR_OE     0x02  	// Overrun error indicator
#define UART_LSR_DR     0x01  	// Receiver data ready

// Modem Status register bits

#define UART_MSR_DCD  0x80  	// Data carrier detect
#define UART_MSR_RI   0x40  	// Ring indicator
#define UART_MSR_DSR  0x20  	// Data set ready
#define UART_MSR_CTS  0x10  	// Clear to send
#define UART_MSR_DDCD 0x08  	// Delta data carrier detect
#define UART_MSR_TERI 0x04  	// Trailing edge ring indicator
#define UART_MSR_DDSR 0x02  	// Delta data set read
#define UART_MSR_DCTS 0x01  	// Delta clear to send



// Module class for the UART.

class UartSC
: public sc_core::sc_module
{
 public:

  // Constructor and destructor

  UartSC( sc_core::sc_module_name  name );
  ~UartSC();

  // Blocking transport method - called whenever an initiator wants to read or
  // write from/to our target port.

  void  doReadWrite( tlm::tlm_generic_payload &payload,
		     sc_core::sc_time         &delayTime );

  // Target port for devices to read or write to us. Initator port, so we can
  // drive an external terminal.
  
  tlm_utils::simple_target_socket<UartSC>     uartInPort;
  tlm_utils::simple_initiator_socket<UartSC>  uartOutPort;


 private:

  // Split out read and write for clarity

  void  doRead( tlm::tlm_generic_payload &payload,
		sc_core::sc_time         &delayTime );

  void  doWrite( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime );

  // Control the UART

  void  uartInit();

  // Talk to the xterm

  int   remoteRead();
  void  remoteWrite( unsigned char  ch );

  // UART registers and additional state

  unsigned char       regs[UART_SIZE];

  unsigned short int  divisorLatch;	// Have to hold this outside
  unsigned char       lastReadByte;	// Got from xterm

  // UART status info

  bool  isLittleEndian;			// Is ISS little endian?

};	// UartSC()


#endif	// UART_SC__H


// EOF
