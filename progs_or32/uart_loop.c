/* ----------------------------------------------------------------------------
 *
 *                  CONFIDENTIAL AND PROPRIETARY INFORMATION
 *                  ========================================
 *
 * Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.
 *
 * This file contains confidential and proprietary information of Embecosm and
 * is protected by copyright, trade secret and other regional, national and
 * international laws, and may be embodied in patents issued or pending.
 *
 * Receipt or possession of this file does not convey any rights to use,
 * reproduce, disclose its contents, or to manufacture, or sell anything it may
 * describe.
 *
 * Reproduction, disclosure or use without specific written authorization of
 * Embecosm is strictly forbidden.
 *
 * Reverse engineering is prohibited.
 *
 * ----------------------------------------------------------------------------
 *
 * Simple loopback test of the external UART.
 *
 * $Id$
 *
 */

#include "utils.h"

#define BASEADDR  0x90000000

struct uart16450
{
  volatile unsigned char  buf;		// R/W: Rx & Tx buffer when DLAB=0  
  volatile unsigned char  ier;		// R/W: Interrupt Enable Register   
  volatile unsigned char  iir;		// R: Interrupt ID Register	    
  volatile unsigned char  lcr;		// R/W: Line Control Register	    
  volatile unsigned char  mcr;		// W: Modem Control Register	    
  volatile unsigned char  lsr;		// R: Line Status Register	    
  volatile unsigned char  msr;		// R: Modem Status Register	    
  volatile unsigned char  scr;		// R/W: Scratch Register	    
};

#define UART_LSR_DR     0x01		// Receiver data ready
#define UART_LCR_DLAB   0x80		// Divisor latch access bit


main()
{
  volatile struct uart16450 *uart = (struct uart16450 *)BASEADDR;
  unsigned short int         divisor;

  divisor = 100000000/9600;

  // Initialize the UART baud rate

  uart->lcr |= UART_LCR_DLAB;
  uart->buf  = (unsigned char)( divisor       & 0x00ff);
  uart->ier  = (unsigned char)((divisor >> 8) & 0x00ff);
  uart->lcr &= ~UART_LCR_DLAB;

  while( 1 ) {

    // Loop until a char is available

    unsigned char  lsr;
    unsigned char  ch;

    do {
      lsr = uart->lsr;
    } while( UART_LSR_DR != (lsr & UART_LSR_DR) );

    ch = uart->buf;

    // Log and write the char back

    simputs( "Read: '" );
    simputc( ch );
    simputs( "'\n" );

    uart->buf = ch;
  }
}
