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

// Offsets for the 16450 UART registers

#define UART_BUF  0		// R/W: Rx/Tx buffer, DLAB=0
#define UART_IER  1 		// R/W: Interrupt Enable Register, DLAB=0
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

#define UART_LCR_MASK   0x03	// Mask for word length
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



// Module class for the UART. It has a simple target socket for the CPU to
// read/write registers, a signal port (rx pin) for the terminal to write
// characters to the UART and a signal port (tx pin) for the UART to write
// characters to the terminal.

class UartSC
: public sc_core::sc_module
{
 public:

  // Constructor and destructor

  UartSC( sc_core::sc_module_name  name,
	  unsigned long int        clockRate );
  ~UartSC();

  // Target socket for the CPU to read or write to us.
  
  tlm_utils::simple_target_socket<UartSC>     uartPort;

  // Fifos for the terminal to read/write to us

  sc_core::sc_fifo_in<unsigned char>   uartRx;
  sc_core::sc_fifo_out<unsigned char>  uartTx;


 private:

  // Control the UART

  void  uartInit( unsigned long int  clockRate );

  // The two thread running the behavior of the UART. The CPU thread is
  // notified when a value appears in the Tx buffer, transfers it to the Tx
  // hold register waits the time start, data and stop bits would take and
  // then writes the value to the terminal. The terminal thread is statically
  // sensitive to the Rx port and copies any data received into the Rx buffer

  void  uartCpuThread();
  void  uartTermThread();

  // Blocking transport method - called whenever the CPU initiator wants to
  // read or write from/to our target port.

  void  cpuReadWrite( tlm::tlm_generic_payload &payload,
		      sc_core::sc_time         &delayTime );

  // Split out CPU read and write for clarity

  void  cpuRead( tlm::tlm_generic_payload &payload,
		sc_core::sc_time         &delayTime );

  void  cpuWrite( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime );

  // Utitlity routine to work out the char delay

  void  uartResetCharDelay();

  // UART event triggered by the CPU writing into the Tx buffer.

  sc_core::sc_event  txReceived;	// The event

  // UART registers and internal state

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

  struct {
    unsigned long int   clockRate;	// External clock into UART
    unsigned short int  divLatch;	// Divisor for ext clock
    sc_core::sc_time    charDelay;	// Total time to Tx a char
  } iState;

  // UART status info

  bool  isLittleEndian;			// Is ISS little endian?

};	// UartSC()


#endif	// UART_SC__H


// EOF
