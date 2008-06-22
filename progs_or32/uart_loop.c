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

#define BASEADDR   0x90000000
#define BAUD_RATE        9600
#define CLOCK_RATE  100000000		// 100 Mhz

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

#define UART_LSR_TEMT   0x40		// Transmitter serial register empty
#define UART_LSR_THRE   0x20		// Transmitter holding register empty
#define UART_LSR_DR     0x01		// Receiver data ready

#define UART_LCR_DLAB   0x80		// Divisor latch access bit


/* Utility routines to set and get flags */

inline void  set( volatile unsigned char *reg,
		           unsigned char  flag )
{
  unsigned char  tmp = *reg;
  *reg = tmp | flag;

}	/* set() */


inline void  clr( volatile unsigned char *reg,
		           unsigned char  flag )
{
  unsigned char  tmp = *reg;
  *reg = tmp & ~flag;

}	/* set() */


inline int  is_set( volatile unsigned char  reg,
		             unsigned char  flag )
{
  unsigned char  tmp = reg & flag;
  return  flag == tmp;

}	/* is_set() */


inline int  is_clr( volatile unsigned char  reg,
		             unsigned char  flag )
{
  unsigned char  tmp = reg & flag;
  return  flag != tmp;

}	/* is_clr() */


main()
{
  volatile struct uart16450 *uart = (struct uart16450 *)BASEADDR;
  unsigned short int         divisor;

  divisor = CLOCK_RATE/16/BAUD_RATE;		// DL is for 16x baud rate

  set( &(uart->lcr), UART_LCR_DLAB );		// Set the divisor latch
  uart->buf  = (unsigned char)( divisor       & 0x00ff);
  uart->ier  = (unsigned char)((divisor >> 8) & 0x00ff);
  clr( &(uart->lcr), UART_LCR_DLAB );

  // Loop echoing characters

  while( 1 ) {
    unsigned char  ch;

    do {			// Loop until a char is available
      ;
    } while( is_clr(uart->lsr, UART_LSR_DR) );

    ch = uart->buf;

    simputs( "Read: '" );	// Log what was read
    simputc( ch );
    simputs( "'\n" );

    do {			// Loop until the trasmit register is free
      ;
    } while( is_clr( uart->lsr, UART_LSR_TEMT | UART_LSR_THRE ) );
      
    uart->buf = ch;
  }
}
