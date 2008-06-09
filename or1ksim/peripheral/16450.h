/* 16450.h -- Definition of types and structures for 8250/16450 serial UART
   Copyright (C) 2000 Damjan Lampret, lampret@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Prototypes */
void uart_reset();
void uart_status();

/* Definitions */
#define UART_ADDR_SPACE   (8)         /* UART memory address space size in bytes */
#define UART_MAX_FIFO_LEN (16)        /* rx FIFO for uart 16550 */
#define MAX_SKEW          (1)         /* max. clock skew in subclocks */
#define UART_VAPI_BUF_LEN 128         /* Size of VAPI command buffer - VAPI should not send more
                                         that this amout of char before requesting something back */
#define UART_CLOCK_DIVIDER 16         /* Uart clock divider */
#define UART_FGETC_SLOWDOWN	100   /* fgetc slowdown factor */

/* Registers */

struct dev_16450 {
  struct {
    uint8_t txbuf[UART_MAX_FIFO_LEN];
    uint16_t rxbuf[UART_MAX_FIFO_LEN]; /* Upper 8-bits is the LCR modifier */
    uint8_t dll;
    uint8_t dlh;
    uint8_t ier;
    uint8_t iir;
    uint8_t fcr;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr;
    uint8_t msr;
    uint8_t scr;
  } regs;   /* Visible registers */
  struct {
    uint8_t txser;    /* Character just sending */
    uint16_t rxser;    /* Character just receiving */
    uint8_t loopback;
  } iregs;  /* Internal registers */
  struct {
    int txbuf_head;   
    int txbuf_tail;
    int rxbuf_head;
    int rxbuf_tail;
    unsigned int txbuf_full;
    unsigned int rxbuf_full;
    int receiveing; /* Receiveing a char */
    int recv_break; /* Receiveing a break */
    int ints; /* Which interrupts are pending */
  } istat;  /* Internal status */

  /* Clocks per char */
  unsigned long char_clks;  
  
  /* VAPI internal registers */
  struct {
    unsigned long char_clks;
    uint8_t dll, dlh;
    uint8_t lcr;
    int skew;
  } vapi;
  
  /* Required by VAPI - circular buffer */
 unsigned long vapi_buf[UART_VAPI_BUF_LEN];  /* Buffer to store incoming characters to,
                                          since we cannot handle them so fast - we
                                          are serial */
  int vapi_buf_head_ptr;               /* Where we write to */
  int vapi_buf_tail_ptr;               /* Where we read from */
  
  /* Length of FIFO, 16 for 16550, 1 for 16450 */
  int fifo_len;

  struct channel *channel;

  /* Configuration */
  int enabled;
  int jitter;
  oraddr_t baseaddr;
  int irq;
  unsigned long vapi_id;
  int uart16550;
  char *channel_str;
};

/*
 * Addresses of visible registers
 *
 */
#define UART_RXBUF  0 /* R: Rx buffer, DLAB=0 */
#define UART_TXBUF  0 /* W: Tx buffer, DLAB=0 */
#define UART_DLL  0 /* R/W: Divisor Latch Low, DLAB=1 */
#define UART_DLH  1 /* R/W: Divisor Latch High, DLAB=1 */
#define UART_IER  1 /* R/W: Interrupt Enable Register */
#define UART_IIR  2 /* R: Interrupt ID Register */
#define UART_FCR  2 /* W: FIFO Control Register */
#define UART_LCR  3 /* R/W: Line Control Register */
#define UART_MCR  4 /* W: Modem Control Register */
#define UART_LSR  5 /* R: Line Status Register */
#define UART_MSR  6 /* R: Modem Status Register */
#define UART_SCR  7 /* R/W: Scratch Register */

/*
 * R/W masks for valid bits in 8250/16450 (mask out 16550 and later bits)
 *
 */
#define UART_VALID_LCR  0xff
#define UART_VALID_LSR  0xff
#define UART_VALID_IIR  0x0f
#define UART_VALID_FCR  0xc0
#define UART_VALID_IER  0x0f
#define UART_VALID_MCR  0x1f
#define UART_VALID_MSR  0xff

/*
 * Bit definitions for the Line Control Register
 * 
 */
#define UART_LCR_DLAB 0x80  /* Divisor latch access bit */
#define UART_LCR_SBC  0x40  /* Set break control */
#define UART_LCR_SPAR 0x20  /* Stick parity (?) */
#define UART_LCR_EPAR 0x10  /* Even parity select */
#define UART_LCR_PARITY 0x08  /* Parity Enable */
#define UART_LCR_STOP 0x04  /* Stop bits: 0=1 stop bit, 1= 2 stop bits */
#define UART_LCR_WLEN5  0x00  /* Wordlength: 5 bits */
#define UART_LCR_WLEN6  0x01  /* Wordlength: 6 bits */
#define UART_LCR_WLEN7  0x02  /* Wordlength: 7 bits */
#define UART_LCR_WLEN8  0x03  /* Wordlength: 8 bits */
#define UART_LCR_RESET  0x03
/*
 * Bit definitions for the Line Status Register
 */
#define UART_LSR_RXERR  0x80  /* Error in rx fifo */
#define UART_LSR_TXSERE 0x40  /* Transmitter serial register empty */
#define UART_LSR_TXBUFE 0x20  /* Transmitter buffer register empty */
#define UART_LSR_BREAK  0x10  /* Break interrupt indicator */
#define UART_LSR_FRAME  0x08  /* Frame error indicator */
#define UART_LSR_PARITY 0x04  /* Parity error indicator */
#define UART_LSR_OVRRUN 0x02  /* Overrun error indicator */
#define UART_LSR_RDRDY  0x01  /* Receiver data ready */

/*
 * Bit definitions for the Interrupt Identification Register
 */
#define UART_IIR_NO_INT 0x01  /* No interrupts pending */
#define UART_IIR_ID 0x06  /* Mask for the interrupt ID */

#define UART_IIR_MSI  0x00  /* Modem status interrupt (Low priority) */
#define UART_IIR_THRI 0x02  /* Transmitter holding register empty */
#define UART_IIR_RDI  0x04  /* Receiver data interrupt */
#define UART_IIR_RLSI 0x06  /* Receiver line status interrupt (High p.) */
#define UART_IIR_CTI  0x0c  /* Character timeout */

/*
 * Bit Definitions for the FIFO Control Register
 */
#define UART_FCR_FIE  0x01  /* FIFO enable */
#define UART_FCR_RRXFI 0x02 /* Reset rx FIFO */
#define UART_FCR_RTXFI 0x04 /* Reset tx FIFO */
#define UART_FIFO_TRIGGER(x) /* Trigger values for indexes 0..3 */\
  ((x) == 0 ? 1\
  :(x) == 1 ? 4\
  :(x) == 2 ? 8\
  :(x) == 3 ? 14 : 0)

/*
 * Bit definitions for the Interrupt Enable Register
 */
#define UART_IER_MSI  0x08  /* Enable Modem status interrupt */
#define UART_IER_RLSI 0x04  /* Enable receiver line status interrupt */
#define UART_IER_THRI 0x02  /* Enable Transmitter holding register int. */
#define UART_IER_RDI  0x01  /* Enable receiver data interrupt */

/*
 * Bit definitions for the Modem Control Register
 */
#define UART_MCR_LOOP 0x10  /* Enable loopback mode */
#define UART_MCR_AUX2 0x08  /* Auxilary 2  */
#define UART_MCR_AUX1 0x04  /* Auxilary 1 */
#define UART_MCR_RTS  0x02  /* Force RTS */
#define UART_MCR_DTR  0x01  /* Force DTR */

/*
 * Bit definitions for the Modem Status Register
 */
#define UART_MSR_DCD  0x80  /* Data Carrier Detect */
#define UART_MSR_RI   0x40  /* Ring Indicator */
#define UART_MSR_DSR  0x20  /* Data Set Ready */
#define UART_MSR_CTS  0x10  /* Clear to Send */
#define UART_MSR_DDCD 0x08  /* Delta DCD */
#define UART_MSR_TERI 0x04  /* Trailing edge ring indicator */
#define UART_MSR_DDSR 0x02  /* Delta DSR */
#define UART_MSR_DCTS 0x01  /* Delta CTS */

/*
 * Various definitions
 */
#define UART_BREAK_COUNT  (1) /* # of chars to count when performing break */
#define UART_CHAR_TIMEOUT (4) /* # of chars to count when performing timeout int. */
