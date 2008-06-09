/* 16450.c -- Simulation of 8250/16450 serial UART
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org

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

/* This is functional simulation of 8250/16450 UARTs. Since we RX/TX data
   via file streams, we can't simulate modem control lines coming from the
   DCE and similar details of communication with the DCE.

   This simulated UART device is intended for basic UART device driver
   verification. From device driver perspective this device looks like a
   regular UART but never reports any modem control lines changes (the
   only DCE responses are incoming characters from the file stream).
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "16450.h"
#include "sim-config.h"
#include "pic.h"
#include "vapi.h"
#include "sched.h"
#include "channel.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(uart);

#define MIN(a,b) ((a) < (b) ? (a) : (b))

void uart_recv_break(void *dat);
void uart_recv_char(void *dat);
void uart_check_vapi(void *dat);
void uart_check_char(void *dat);
static void uart_sched_recv_check(struct dev_16450 *uart);
static void uart_vapi_cmd(void *dat);
static void uart_clear_int(struct dev_16450 *uart, int intr);
void uart_tx_send(void *dat);

/* Number of clock cycles (one clock cycle is when UART_CLOCK_DIVIDER simulator
 * cycles have elapsed) before a single character is transmitted or received. */
static unsigned long char_clks(int dll, int dlh, int lcr)
{
  unsigned int bauds_per_char = 2;
  unsigned long char_clks = ((dlh << 8) + dll);
  
  if (lcr & UART_LCR_PARITY)
    bauds_per_char += 2;

  /* stop bits 1 or two */
  if (lcr & UART_LCR_STOP)
    bauds_per_char += 4;
  else
    if ((lcr & 0x3) != 0)
      bauds_per_char += 2;
    else 
      bauds_per_char += 3;
  
  bauds_per_char += 10 + ((lcr & 0x3) << 1);

  return (char_clks * bauds_per_char) >> 1;
}

/*---------------------------------------------------[ Interrupt handling ]---*/
/* Signals the specified interrupt.  If a higher priority interrupt is already
 * pending, do nothing */
static void uart_int_msi(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_MSI;

  if(!(uart->regs.ier & UART_IER_MSI))
    return;

  if((uart->regs.iir & UART_IIR_NO_INT) || (uart->regs.iir == UART_IIR_MSI)) {
    TRACE("Raiseing modem status interrupt\n");

    if((uart->regs.iir != UART_IIR_MSI) && (uart->regs.iir != UART_IIR_NO_INT)) {
      uart_clear_int(uart, uart->regs.iir);
      uart->regs.iir = UART_IIR_MSI;
    } else {
      uart->regs.iir = UART_IIR_MSI;
      SCHED_ADD(uart_int_msi, dat, UART_CLOCK_DIVIDER);
      report_interrupt(uart->irq);
    }
  }
}

static void uart_int_thri(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_THRI;

  if(!(uart->regs.ier & UART_IER_THRI))
    return;

  if((uart->regs.iir & UART_IIR_NO_INT) || (uart->regs.iir == UART_IIR_MSI) ||
     (uart->regs.iir == UART_IIR_THRI)) {
    TRACE("Raiseing transmitter holding register interrupt\n");
 
    if((uart->regs.iir != UART_IIR_THRI) && (uart->regs.iir != UART_IIR_NO_INT)) {
      uart_clear_int(uart, uart->regs.iir);
      uart->regs.iir = UART_IIR_THRI;
    } else {
      uart->regs.iir = UART_IIR_THRI;
      SCHED_ADD(uart_int_thri, dat, UART_CLOCK_DIVIDER);
      report_interrupt(uart->irq);
    }
  }
}

static void uart_int_cti(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_CTI;

  if(!(uart->regs.ier & UART_IER_RDI))
    return;

  if((uart->regs.iir != UART_IIR_RLSI) && (uart->regs.iir != UART_IIR_RDI)) {
    TRACE("Raiseing character timeout interrupt\n");

    if((uart->regs.iir != UART_IIR_CTI) && (uart->regs.iir != UART_IIR_NO_INT)) {
      uart_clear_int(uart, uart->regs.iir);
      uart->regs.iir = UART_IIR_CTI;
    } else {
      uart->regs.iir = UART_IIR_CTI;
      SCHED_ADD(uart_int_cti, dat, UART_CLOCK_DIVIDER);
      report_interrupt(uart->irq);
    }
  }
}

static void uart_int_rdi(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_RDI;

  if(!(uart->regs.ier & UART_IER_RDI))
    return;

  if(uart->regs.iir != UART_IIR_RLSI) {
    TRACE("Raiseing receiver data interrupt\n");

    if((uart->regs.iir != UART_IIR_RDI) && (uart->regs.iir != UART_IIR_NO_INT)) {
      uart_clear_int(uart, uart->regs.iir);
      uart->regs.iir = UART_IIR_RDI;
    } else {
      uart->regs.iir = UART_IIR_RDI;
      SCHED_ADD(uart_int_rdi, dat, UART_CLOCK_DIVIDER);
      report_interrupt(uart->irq);
    }
  }
}

static void uart_int_rlsi(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.ints |= 1 << UART_IIR_RLSI;

  if(!(uart->regs.ier & UART_IER_RLSI))
    return;

  TRACE("Raiseing receiver line status interrupt\n");

  /* Highest priority interrupt */
  if((uart->regs.iir != UART_IIR_RLSI) && (uart->regs.iir != UART_IIR_NO_INT)) {
    uart_clear_int(uart, uart->regs.iir);
    uart->regs.iir = UART_IIR_RLSI;
  } else {
    uart->regs.iir = UART_IIR_RLSI;
    SCHED_ADD(uart_int_rlsi, dat, UART_CLOCK_DIVIDER);
    report_interrupt(uart->irq);
  }
}

/* Checks to see if an RLSI interrupt is due and schedules one if need be */
static void uart_check_rlsi(void *dat)
{
  struct dev_16450 *uart = dat;

  if(uart->regs.lsr & (UART_LSR_OVRRUN | UART_LSR_PARITY | UART_LSR_FRAME |
                       UART_LSR_BREAK))
    uart_int_rlsi(uart);
}

/* Checks to see if an RDI interrupt is due and schedules one if need be */
static void uart_check_rdi(void *dat)
{
  struct dev_16450 *uart = dat;

  if(uart->istat.rxbuf_full >= UART_FIFO_TRIGGER(uart->regs.fcr >> 6)) {
    TRACE("FIFO trigger level reached %i\n",
          UART_FIFO_TRIGGER(uart->regs.fcr >> 6));
    uart_int_rdi(uart);
  }
}

/* Raises the next highest priority interrupt */
static void uart_next_int(void *dat)
{
  struct dev_16450 *uart = dat;

  /* Interrupt detection in proper priority order. */
  if((uart->istat.ints & (1 << UART_IIR_RLSI)) &&
     (uart->regs.ier & UART_IER_RLSI))
    uart_int_rlsi(uart);
  else if((uart->istat.ints & (1 << UART_IIR_RDI)) &&
          (uart->regs.ier & UART_IER_RDI))
    uart_int_rdi(uart);
  else if((uart->istat.ints & (1 << UART_IIR_CTI)) &&
          (uart->regs.ier & UART_IER_RDI))
    uart_int_cti(uart);
  else if((uart->istat.ints & (1 << UART_IIR_THRI)) &&
          (uart->regs.ier & UART_IER_THRI))
    uart_int_thri(uart);
  else if((uart->istat.ints & (1 << UART_IIR_MSI)) &&
          (uart->regs.ier & UART_IER_MSI))
    uart_int_msi(uart);
  else
    uart->regs.iir = UART_IIR_NO_INT;
}

/* Clears potentially pending interrupts */
static void uart_clear_int(struct dev_16450 *uart, int intr)
{
  uart->istat.ints &= ~(1 << intr);

  TRACE("Interrupt pending was %x\n", uart->regs.iir);

  /* Short-circuit most likely case */
  if(uart->regs.iir == UART_IIR_NO_INT)
    return;

  if(intr != uart->regs.iir)
    return;

  TRACE("Clearing interrupt 0x%x\n", intr);

  uart->regs.iir = UART_IIR_NO_INT;

  switch(intr) {
  case UART_IIR_RLSI:
    SCHED_FIND_REMOVE(uart_int_rlsi, uart);
    break;
  case UART_IIR_RDI:
    SCHED_FIND_REMOVE(uart_int_rdi, uart);
    break;
  case UART_IIR_CTI:
    SCHED_FIND_REMOVE(uart_int_cti, uart);
    break;
  case UART_IIR_THRI:
    SCHED_FIND_REMOVE(uart_int_thri, uart);
    break;
  case UART_IIR_MSI:
    SCHED_FIND_REMOVE(uart_int_msi, uart);
    break;
  }

  /* Schedule this job as there is no rush to send the next interrupt (the or.
   * code is probably still running with interrupts disabled and this function
   * is called from the uart_{read,write}_byte functions. */
  SCHED_ADD(uart_next_int, uart, 0);
}

/*----------------------------------------------------[ Loopback handling ]---*/
static void uart_loopback(struct dev_16450 *uart)
{
  if(!(uart->regs.mcr & UART_MCR_LOOP))
    return;

  if((uart->regs.mcr & UART_MCR_AUX2) != ((uart->regs.msr & UART_MSR_DCD) >> 4))
    uart->regs.msr |= UART_MSR_DDCD;

  if((uart->regs.mcr & UART_MCR_AUX1) < ((uart->regs.msr & UART_MSR_RI) >> 4))
    uart->regs.msr |= UART_MSR_TERI;

  if((uart->regs.mcr & UART_MCR_RTS) != ((uart->regs.msr & UART_MSR_CTS) >> 3))
    uart->regs.msr |= UART_MSR_DCTS;

  if((uart->regs.mcr & UART_MCR_DTR) != ((uart->regs.msr & UART_MSR_DSR) >> 5))
    uart->regs.msr |= UART_MSR_DDSR;

  uart->regs.msr &= ~(UART_MSR_DCD | UART_MSR_RI | UART_MSR_DSR | UART_MSR_CTS);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_AUX2) << 4);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_AUX1) << 4);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_RTS) << 3);
  uart->regs.msr |= ((uart->regs.mcr & UART_MCR_DTR) << 5);

  if(uart->regs.msr & (UART_MSR_DCTS | UART_MSR_DDSR | UART_MSR_TERI |
                       UART_MSR_DDCD))
    uart_int_msi(uart);
}

/*----------------------------------------------------[ Transmitter logic ]---*/
/* Sends the data in the shift register to the outside world */
static void send_char (struct dev_16450 *uart, int bits_send)
{
  PRINTF ("%c", (char)uart->iregs.txser);
  TRACE("TX \'%c\' via UART at %"PRIxADDR"\n", (char)uart->iregs.txser,
        uart->baseaddr);
  if (uart->regs.mcr & UART_MCR_LOOP)
    uart->iregs.loopback = uart->iregs.txser;
  else {
    /* Send to either VAPI or to file */
    if (uart->vapi_id) {
      int par, pe, fe, nbits;
      int j, data;
      unsigned long packet = 0;

      nbits = MIN (bits_send, (uart->regs.lcr & UART_LCR_WLEN8) + 5);
      /* Encode a packet */
      packet = uart->iregs.txser & ((1 << nbits) - 1);

      /* Calculate parity */
      for (j = 0, par = 0; j < nbits; j++)
        par ^= (packet >> j) & 1;

      if (uart->regs.lcr & UART_LCR_PARITY) {
        if (uart->regs.lcr & UART_LCR_SPAR) {
          packet |= 1 << nbits;
        } else {
          if (uart->regs.lcr & UART_LCR_EPAR)
            packet |= par << nbits;
          else
            packet |= (par ^ 1) << nbits;
        }
        nbits++;
      }
      packet |= 1 << (nbits++);
      if (uart->regs.lcr & UART_LCR_STOP)
        packet |= 1 << (nbits++);
        
      /* Decode a packet */
      nbits = (uart->vapi.lcr & UART_LCR_WLEN8) + 5;
      data = packet & ((1 << nbits) - 1);
      
      /* Calculate parity, including parity bit */
      for (j = 0, par = 0; j < nbits + 1; j++)
        par ^= (packet >> j) & 1;
      
      if (uart->vapi.lcr & UART_LCR_PARITY) {
        if (uart->vapi.lcr & UART_LCR_SPAR) {
          pe = !((packet >> nbits) & 1);
        } else {
          if (uart->vapi.lcr & UART_LCR_EPAR)
            pe = par != 0;
          else
            pe = par != 1;
        }
        nbits++;
      } else
        pe = 0;
        
      fe = ((packet >> (nbits++)) & 1) ^ 1;
      if (uart->vapi.lcr & UART_LCR_STOP)
        fe |= ((packet >> (nbits++)) & 1) ^ 1;

      TRACE ("lcr vapi %02x, uart %02x\n", uart->vapi.lcr, uart->regs.lcr);
      data |= (uart->vapi.lcr << 8) | (pe << 16) | (fe << 17) | (uart->vapi.lcr << 8);
      TRACE ("vapi_send (%08lx, %08x)\n", uart->vapi_id, data);
      vapi_send (uart->vapi_id, data);
    } else {
      char buffer[1] = { uart->iregs.txser & 0xFF };
      channel_write(uart->channel, buffer, 1);
    }
  }
}

/* Called when all the bits have been shifted out of the shift register */
void uart_char_clock(void *dat)
{
  struct dev_16450 *uart = dat;

  TRACE("Sending data in shift reg: 0x%02"PRIx8"\n", uart->iregs.txser);
  /* We've sent all bits */
  send_char(uart, (uart->regs.lcr & UART_LCR_WLEN8) + 5); 

  if(!uart->istat.txbuf_full)
    uart->regs.lsr |= UART_LSR_TXSERE;
  else
    uart_tx_send(uart);
}

/* Called when a break has been shifted out of the shift register */
void uart_send_break(void *dat)
{
  struct dev_16450 *uart = dat;

  TRACE("Sending break\n");
#if 0
  /* Send broken frame */
  int nbits_sent = ((uart->regs.lcr & UART_LCR_WLEN8) + 5) * (uart->istat.txser_clks - 1) / uart->char_clks;
  send_char(i, nbits_sent);
#endif
  /* Send one break signal */
  vapi_send (uart->vapi_id, UART_LCR_SBC << 8);

  /* Send the next char (if there is one) */
  if(!uart->istat.txbuf_full)
    uart->regs.lsr |= UART_LSR_TXSERE;
  else
    uart_tx_send(uart);
}

/* Scheduled whenever the TX buffer has characters in it and we aren't sending
 * a character. */
void uart_tx_send(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->iregs.txser = uart->regs.txbuf[uart->istat.txbuf_tail];
  uart->istat.txbuf_tail = (uart->istat.txbuf_tail + 1) % uart->fifo_len;
  uart->istat.txbuf_full--;
  uart->regs.lsr &= ~UART_LSR_TXSERE;

  TRACE("Moveing head of TX fifo (fill: %i) to shift reg 0x%02"PRIx8"\n", 
        uart->istat.txbuf_full, uart->iregs.txser);

  /* Schedules a char_clock to run in the correct amount of time */
  if(!(uart->regs.lcr & UART_LCR_SBC)) {
    SCHED_ADD(uart_char_clock, uart, uart->char_clks * UART_CLOCK_DIVIDER);
  } else {
    TRACE("Sending break not char\n");
    SCHED_ADD(uart_send_break, uart, 0);
  }

  /* When UART is in either character mode, i.e. 16450 emulation mode, or FIFO
   * mode, the THRE interrupt is raised when THR transitions from full to empty.
   */
  if (!uart->istat.txbuf_full) {
    uart->regs.lsr |= UART_LSR_TXBUFE;
    uart_int_thri(uart);
  }
}

/*-------------------------------------------------------[ Receiver logic ]---*/
/* Adds a character to the RX FIFO */
static void uart_add_char (struct dev_16450 *uart, int ch)
{
  uart->regs.lsr |= UART_LSR_RDRDY;
  uart_clear_int(uart, UART_IIR_CTI);
  SCHED_FIND_REMOVE(uart_int_cti, uart);
  SCHED_ADD(uart_int_cti, uart,
            uart->char_clks * UART_CHAR_TIMEOUT * UART_CLOCK_DIVIDER);

  if (uart->istat.rxbuf_full + 1 > uart->fifo_len) {
    uart->regs.lsr |= UART_LSR_OVRRUN | UART_LSR_RXERR;
    uart_int_rlsi(uart);
  } else {
    TRACE("add %02x\n", ch);
    uart->regs.rxbuf[uart->istat.rxbuf_head] = ch;
    uart->istat.rxbuf_head = (uart->istat.rxbuf_head + 1) % uart->fifo_len;
    if(!uart->istat.rxbuf_full++) {
      uart->regs.lsr |= ch >> 8;
      uart_check_rlsi(uart);
    }
  }
  uart_check_rdi(uart);
}

/* Called when a break sequence is about to start.  It stops receiveing
 * characters and schedules the uart_recv_break to send the break */
void uart_recv_break_start(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.receiveing = 0;
  uart->istat.recv_break = 1;

  SCHED_FIND_REMOVE(uart_recv_char, uart);

  if(uart->vapi_id && (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr))
    uart_vapi_cmd(uart);

  SCHED_ADD(uart_recv_break, uart,
            UART_BREAK_COUNT * uart->vapi.char_clks * UART_CLOCK_DIVIDER);
}

/* Stops sending breaks and starts receiveing characters */
void uart_recv_break_stop(void *dat)
{
  struct dev_16450 *uart = dat;

  uart->istat.recv_break = 0;
  SCHED_FIND_REMOVE(uart_recv_break, dat);
}

/* Receives a break */
void uart_recv_break(void *dat)
{
  struct dev_16450 *uart = dat;
  unsigned lsr = UART_LSR_BREAK | UART_LSR_RXERR | UART_LSR_RDRDY;

  uart_add_char(uart, lsr << 8);
}

/* Moves a character from the serial register to the RX FIFO */
void uart_recv_char(void *dat)
{
  struct dev_16450 *uart = dat;
  uint16_t char_to_add;

  /* Set unused character bits to zero and allow lsr register in fifo */
  char_to_add = uart->iregs.rxser & (((1 << ((uart->regs.lcr & 3) + 5)) - 1) | 0xff00);

  TRACE("Receiving 0x%02"PRIx16"'%c' via UART at %"PRIxADDR"\n",
              char_to_add, (char)char_to_add, uart->baseaddr);
  PRINTF ("%c", (char)char_to_add);

  if (uart->regs.mcr & UART_MCR_LOOP) {
    uart->iregs.rxser = uart->iregs.loopback;
    uart->istat.receiveing = 1;
    SCHED_ADD(uart_recv_char, uart, uart->char_clks * UART_CLOCK_DIVIDER);
  } else {
    uart->istat.receiveing = 0;
    uart_sched_recv_check(uart);
    if(uart->vapi_id && (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr))
      SCHED_ADD(uart_vapi_cmd, uart, 0);
  }

  uart_add_char(uart, char_to_add);
}

/* Checks if there is a character waiting to be received */
void uart_check_char(void *dat)
{
  struct dev_16450 *uart = dat;
  uint8_t buffer;
  int retval;

  /* Check if there is something waiting, and put it into rxser */
  retval = channel_read(uart->channel, (char *)&buffer, 1);
  if(retval > 0) {
    TRACE("Shifting 0x%02"PRIx8" (`%c') into shift reg\n", buffer, buffer);
    uart->iregs.rxser = buffer;
    uart->istat.receiveing = 1;
    SCHED_ADD(uart_recv_char, uart, uart->char_clks * UART_CLOCK_DIVIDER);
    return;
  }

  if(!retval) {
    SCHED_ADD(uart_check_char, uart, UART_FGETC_SLOWDOWN * UART_CLOCK_DIVIDER);
    return;
  }

  if(retval < 0)
    perror(uart->channel_str);
}

static void uart_sched_recv_check(struct dev_16450 *uart)
{
  if(!uart->vapi_id)
    SCHED_ADD(uart_check_char, uart, UART_FGETC_SLOWDOWN * UART_CLOCK_DIVIDER);
}

/*----------------------------------------------------[ UART I/O handling ]---*/
/* Set a specific UART register with value. */
void uart_write_byte(oraddr_t addr, uint8_t value, void *dat)
{
  struct dev_16450 *uart = dat;
  
  if (uart->regs.lcr & UART_LCR_DLAB) {
    switch (addr) {
      case UART_DLL:
        uart->regs.dll = value;
        uart->char_clks = char_clks(uart->regs.dll, uart->regs.dlh, uart->regs.lcr);
        TRACE("\tSetting char_clks to %li (%02x, %02x, %02x)\n", uart->char_clks,
              uart->regs.dll, uart->regs.dlh, uart->regs.lcr);
        return;
      case UART_DLH:
        TRACE("Setting dlh with %"PRIx8"\n", value);
        uart->regs.dlh = value;
        return;
    }
  }
  
  switch (addr) {
    case UART_TXBUF:
      TRACE("Adding %"PRIx8" to TX FIFO (fill %i)\n", value,
            uart->istat.txbuf_full);
      uart->regs.lsr &= ~UART_LSR_TXBUFE;
      if (uart->istat.txbuf_full < uart->fifo_len) {
        uart->regs.txbuf[uart->istat.txbuf_head] = value;
        uart->istat.txbuf_head = (uart->istat.txbuf_head + 1) % uart->fifo_len;
        if(!uart->istat.txbuf_full++ && (uart->regs.lsr & UART_LSR_TXSERE))
          SCHED_ADD(uart_tx_send, uart, 0);
      } else
        uart->regs.txbuf[uart->istat.txbuf_head] = value;

      uart_clear_int(uart, UART_IIR_THRI);
      break;
    case UART_FCR:
      TRACE("Setting FCR reg with %"PRIx8"\n", value);
      uart->regs.fcr = value & UART_VALID_FCR;
      if ((uart->fifo_len == 1 && (value & UART_FCR_FIE))
       || (uart->fifo_len != 1 && !(value & UART_FCR_FIE)))
        value |= UART_FCR_RRXFI | UART_FCR_RTXFI;
      uart->fifo_len = (value & UART_FCR_FIE) ? 16 : 1;
      if (value & UART_FCR_RTXFI) {
        uart->istat.txbuf_head = uart->istat.txbuf_tail = 0;
        uart->istat.txbuf_full = 0;
        uart->regs.lsr |= UART_LSR_TXBUFE;

        /* For FIFO-mode only, THRE interrupt is set when THR and FIFO are empty
         */
        if(uart->fifo_len == 16)
          SCHED_ADD(uart_int_thri, uart, 0);

        SCHED_FIND_REMOVE(uart_tx_send, uart);
      }
      if (value & UART_FCR_RRXFI) {
        uart->istat.rxbuf_head = uart->istat.rxbuf_tail = 0;
        uart->istat.rxbuf_full = 0;
        uart->regs.lsr &= ~UART_LSR_RDRDY;
        uart_clear_int(uart, UART_IIR_RDI);
        uart_clear_int(uart, UART_IIR_CTI);
      }
      break;
    case UART_IER:
      uart->regs.ier = value & UART_VALID_IER;
      TRACE("Enabling 0x%02x interrupts with 0x%x interrupts pending\n",
            value, uart->istat.ints);
      SCHED_ADD(uart_next_int, uart, 0);
      break;
    case UART_LCR:
      TRACE("Setting LCR reg with %"PRIx8"\n", value);
      if((uart->regs.lcr & UART_LCR_SBC) != (value & UART_LCR_SBC)) {
        if((value & UART_LCR_SBC) && !(uart->regs.lsr & UART_LSR_TXSERE)) {
          /* Schedule a job to send the break char */
          SCHED_FIND_REMOVE(uart_char_clock, uart);
          SCHED_ADD(uart_send_break, uart, 0);
        }
        if(!(value & UART_LCR_SBC) && !(uart->regs.lsr & UART_LSR_TXSERE)) {
          /* Schedule a job to start sending characters */
          SCHED_ADD(uart_tx_send, uart, 0);
          /* Remove the uart_send_break job just in case it has not run yet */
          SCHED_FIND_REMOVE(uart_char_clock, uart);
        }
      }
      uart->regs.lcr = value & UART_VALID_LCR;
      uart->char_clks = char_clks(uart->regs.dll, uart->regs.dlh, uart->regs.lcr);
      break;
    case UART_MCR:
      TRACE("Setting MCR reg with %"PRIx8"\n", value);
      uart->regs.mcr = value & UART_VALID_MCR;
      uart_loopback(uart);
      break;
    case UART_SCR:
      TRACE("Setting SCR reg with %"PRIx8"\n", value);
      uart->regs.scr = value;
      break;
    default:
      TRACE("write out of range (addr %"PRIxADDR")\n", addr);
  }
}

/* Read a specific UART register. */
uint8_t uart_read_byte(oraddr_t addr, void *dat)
{
  struct dev_16450 *uart = dat;
  uint8_t value = 0;
  
  if (uart->regs.lcr & UART_LCR_DLAB) {
    switch (addr) {
      case UART_DLL:
        value = uart->regs.dll;
        TRACE("reading DLL = %"PRIx8"\n", value);
        return value;
      case UART_DLH:
        value = uart->regs.dlh;
        TRACE("reading DLH = %"PRIx8"\n", value);
        return value;
    }
  }
  
  switch (addr) {
    case UART_RXBUF:
      { /* Print out FIFO for debugging */
        int i;
        TRACE("(%i/%i, %i, %i:", uart->istat.rxbuf_full, uart->fifo_len,
              uart->istat.rxbuf_head, uart->istat.rxbuf_tail);
        for (i = 0; i < uart->istat.rxbuf_full; i++)
          TRACE("%02x ",
                uart->regs.rxbuf[(uart->istat.rxbuf_tail + i) % uart->fifo_len]);
        TRACE(")\n");
      }
      if (uart->istat.rxbuf_full) {
        value = uart->regs.rxbuf[uart->istat.rxbuf_tail];
        uart->istat.rxbuf_tail = (uart->istat.rxbuf_tail + 1) % uart->fifo_len;
        uart->istat.rxbuf_full--;
        TRACE("Reading %"PRIx8" out of RX FIFO\n", value);
      } else
        TRACE("Trying to read out of RX FIFO but it's empty!\n");

      uart_clear_int(uart, UART_IIR_RDI);
      uart_clear_int(uart, UART_IIR_CTI);
      SCHED_FIND_REMOVE(uart_int_cti, uart);

      if(uart->istat.rxbuf_full) {
        uart->regs.lsr |= UART_LSR_RDRDY | uart->regs.rxbuf[uart->istat.rxbuf_tail] >> 8;
        SCHED_ADD(uart_int_cti, uart,
                  uart->char_clks * UART_CHAR_TIMEOUT * UART_CLOCK_DIVIDER);
        /* Since we're not allowed to raise interrupts from read/write functions
         * schedule them to run just after we have executed this read
         * instruction. */
        SCHED_ADD(uart_check_rlsi, uart, 0);
        SCHED_ADD(uart_check_rdi, uart, 0);
      } else {
        uart->regs.lsr &= ~UART_LSR_RDRDY;
      }
      break;
    case UART_IER:
      value = uart->regs.ier & UART_VALID_IER;
      TRACE("reading IER = %"PRIx8"\n", value);
      break;
    case UART_IIR:
      value = (uart->regs.iir & UART_VALID_IIR) | 0xc0;
      /* Only clear the thri interrupt if it is the one we are repporting */
      if(uart->regs.iir == UART_IIR_THRI)
        uart_clear_int(uart, UART_IIR_THRI);
      TRACE("reading IIR = %"PRIx8"\n", value);
      break;
    case UART_LCR:
      value = uart->regs.lcr & UART_VALID_LCR;
      TRACE("reading LCR = %"PRIx8"\n", value);
      break;
    case UART_MCR:
      value = 0;
      TRACE("reading MCR = %"PRIx8"\n", value);
      break;
    case UART_LSR:
      value = uart->regs.lsr & UART_VALID_LSR;
      uart->regs.lsr &=
        ~(UART_LSR_OVRRUN | UART_LSR_BREAK | UART_LSR_PARITY
         | UART_LSR_FRAME | UART_LSR_RXERR);
      /* Clear potentially pending RLSI interrupt */
      uart_clear_int(uart, UART_IIR_RLSI);
      TRACE("reading LSR = %"PRIx8"\n", value);
      break;
    case UART_MSR:
      value = uart->regs.msr & UART_VALID_MSR;
      uart->regs.msr = 0;
      uart_clear_int(uart, UART_IIR_MSI);
      uart_loopback(uart);
      TRACE("reading MSR = %"PRIx8"\n", value);
      break;
    case UART_SCR:
      value = uart->regs.scr;
      TRACE("reading SCR = %"PRIx8"\n", value);
      break;
    default:
      TRACE("read out of range (addr %"PRIxADDR")\n", addr);
  }
  return value;
}

/*--------------------------------------------------------[ VAPI handling ]---*/
/* Decodes the read vapi command */
static void uart_vapi_cmd(void *dat)
{
  struct dev_16450 *uart = dat;
  int received = 0;

  while (!received) {
    if (uart->vapi_buf_head_ptr != uart->vapi_buf_tail_ptr) {
      unsigned long data = uart->vapi_buf[uart->vapi_buf_tail_ptr];
      TRACE("\tHandling: %08lx (%i,%i)\n", data, uart->vapi_buf_head_ptr,
            uart->vapi_buf_tail_ptr);
      uart->vapi_buf_tail_ptr = (uart->vapi_buf_tail_ptr + 1) % UART_VAPI_BUF_LEN;
      switch (data >> 24) {
      case 0x00:
        uart->vapi.lcr = (data >> 8) & 0xff;
        /* Put data into rx fifo */
        uart->iregs.rxser = data & 0xff;
        uart->vapi.char_clks = char_clks (uart->vapi.dll, uart->vapi.dlh, uart->vapi.lcr);
        if((uart->vapi.lcr & ~UART_LCR_SBC) != (uart->regs.lcr & ~UART_LCR_SBC)
            || uart->vapi.char_clks != uart->char_clks
            || uart->vapi.skew < -MAX_SKEW || uart->vapi.skew > MAX_SKEW) {
          if((uart->vapi.lcr & ~UART_LCR_SBC) != (uart->regs.lcr & ~UART_LCR_SBC))
            WARN("unmatched VAPI (%02"PRIx8") and uart (%02"PRIx8") modes.\n",
                 uart->vapi.lcr & ~UART_LCR_SBC, uart->regs.lcr & ~UART_LCR_SBC);
          if(uart->vapi.char_clks != uart->char_clks) {
            WARN("unmatched VAPI (%li) and uart (%li) char clocks.\n",
                 uart->vapi.char_clks, uart->char_clks);
            WARN("VAPI: lcr: %02"PRIx8", dll: %02"PRIx8", dlh: %02"PRIx8"\n",
                 uart->vapi.lcr, uart->vapi.dll, uart->vapi.dlh);
            WARN("UART: lcr: %02"PRIx8", dll: %02"PRIx8", dlh: %02"PRIx8"\n",
                 uart->regs.lcr, uart->regs.dll, uart->vapi.dlh);
          }
          if(uart->vapi.skew < -MAX_SKEW || uart->vapi.skew > MAX_SKEW)
            WARN("VAPI skew is beyond max: %i\n", uart->vapi.skew);
          /* Set error bits */
          uart->iregs.rxser |= (UART_LSR_FRAME | UART_LSR_RXERR) << 8;
          if(uart->regs.lcr & UART_LCR_PARITY)
            uart->iregs.rxser |= UART_LSR_PARITY << 8;
        }
        if(!uart->istat.recv_break) {
          uart->istat.receiveing = 1;
          SCHED_ADD(uart_recv_char, uart, uart->char_clks * UART_CLOCK_DIVIDER);
        }
        received = 1;
        break;
      case 0x01:
        uart->vapi.dll = (data >> 0) & 0xff;
        uart->vapi.dlh = (data >> 8) & 0xff;
        break;
      case 0x02:
        uart->vapi.lcr = (data >> 8) & 0xff;
        break;
      case 0x03:
        uart->vapi.skew = (signed short)(data & 0xffff);
        break;
      case 0x04:
        if((data >> 16) & 1) {
          /* If data & 0xffff is 0 then set the break imediatly and handle the
           * following commands as appropriate */
          if(!(data & 0xffff))
            uart_recv_break_start(uart);
          else
            /* Schedule a job to start sending breaks */
            SCHED_ADD(uart_recv_break_start, uart,
                      (data & 0xffff) * UART_CLOCK_DIVIDER);
        } else {
          /* If data & 0xffff is 0 then release the break imediatly and handle
           * the following commands as appropriate */
          if(!(data & 0xffff))
            uart_recv_break_stop(uart);
          else
            /* Schedule a job to stop sending breaks */
            SCHED_ADD(uart_recv_break_stop, uart,
                      (data & 0xffff) * UART_CLOCK_DIVIDER);
        }
        break;
      default:
        WARN("WARNING: Invalid vapi command %02lx\n", data >> 24);
        break;
      }
    } else break;
  }
}

/* Function that handles incoming VAPI data.  */
void uart_vapi_read (unsigned long id, unsigned long data, void *dat)
{
  struct dev_16450 *uart = dat;
  TRACE("UART: id %08lx, data %08lx\n", id, data);
  uart->vapi_buf[uart->vapi_buf_head_ptr] = data;
  uart->vapi_buf_head_ptr = (uart->vapi_buf_head_ptr + 1) % UART_VAPI_BUF_LEN;
  if (uart->vapi_buf_tail_ptr == uart->vapi_buf_head_ptr) {
    fprintf (stderr, "FATAL: uart VAPI buffer to small.\n");
    exit (1);
  }
  if(!uart->istat.receiveing)
    uart_vapi_cmd(uart);
}

/*--------------------------------------------------------[ Sim callbacks ]---*/
/* Reset.  It initializes all registers of all UART devices to zero values,
 * (re)opens all RX/TX file streams and places devices in memory address
 * space.  */
void uart_reset(void *dat)
{
  struct dev_16450 *uart = dat;

  if(uart->vapi_id) {
    vapi_install_handler(uart->vapi_id, uart_vapi_read, dat);
  } else if (uart->channel_str && uart->channel_str[0]) { /* Try to create stream. */
    if(uart->channel)
      channel_close(uart->channel);
    else
      uart->channel = channel_init(uart->channel_str);
    if(channel_open(uart->channel) < 0) {
      WARN ("WARNING: problem with channel \"%s\" detected.\n", uart->channel_str);
    } else if (config.sim.verbose)
      PRINTF("UART at 0x%"PRIxADDR"\n", uart->baseaddr);
  } else {
    WARN ("WARNING: UART at %"PRIxADDR" has no vapi nor channel specified\n",
          uart->baseaddr);
  }
    
  if (uart->uart16550)
    uart->fifo_len = 16;
  else
    uart->fifo_len = 1;

  uart->istat.rxbuf_head = uart->istat.rxbuf_tail = 0;
  uart->istat.txbuf_head = uart->istat.txbuf_tail = 0;
  
  uart->istat.txbuf_full = uart->istat.rxbuf_full = 0;

  uart->char_clks = 0;

  uart->iregs.txser = 0;
  uart->iregs.rxser = 0;
  uart->iregs.loopback = 0;
  uart->istat.receiveing = 0;
  uart->istat.recv_break = 0;
  uart->istat.ints = 0;

  memset(uart->regs.txbuf, 0, sizeof(uart->regs.txbuf));
  memset(uart->regs.rxbuf, 0, sizeof(uart->regs.rxbuf));

  uart->regs.dll = 0;
  uart->regs.dlh = 0;
  uart->regs.ier = 0;
  uart->regs.iir = UART_IIR_NO_INT;
  uart->regs.fcr = 0;
  uart->regs.lcr = UART_LCR_RESET;
  uart->regs.mcr = 0;
  uart->regs.lsr = UART_LSR_TXBUFE | UART_LSR_TXSERE;
  uart->regs.msr = 0;
  uart->regs.scr = 0;

  uart->vapi.skew = 0;
  uart->vapi.lcr = 0;
  uart->vapi.dll = 0;
  uart->vapi.char_clks = 0;

  uart->vapi_buf_head_ptr = 0;
  uart->vapi_buf_tail_ptr = 0;
  memset(uart->vapi_buf, 0, sizeof(uart->vapi_buf));

  uart_sched_recv_check(uart);
}

/* Print register values on stdout. */
void uart_status(void *dat)
{
  struct dev_16450 *uart = dat;
  int i;
  
  PRINTF("\nUART visible registers at 0x%"PRIxADDR":\n", uart->baseaddr);
  PRINTF("RXBUF: ");
  for (i = uart->istat.rxbuf_head; i != uart->istat.rxbuf_tail; i = (i + 1) % uart->fifo_len)
    PRINTF (" %.2x", uart->regs.rxbuf[i]);
  PRINTF("TXBUF: ");
  for (i = uart->istat.txbuf_head; i != uart->istat.txbuf_tail; i = (i + 1) % uart->fifo_len)
    PRINTF (" %.2x", uart->regs.txbuf[i]);
  PRINTF("\n");
  PRINTF("DLL  : %.2x  DLH  : %.2x\n", uart->regs.dll, uart->regs.dlh);
  PRINTF("IER  : %.2x  IIR  : %.2x\n", uart->regs.ier, uart->regs.iir);
  PRINTF("LCR  : %.2x  MCR  : %.2x\n", uart->regs.lcr, uart->regs.mcr);
  PRINTF("LSR  : %.2x  MSR  : %.2x\n", uart->regs.lsr, uart->regs.msr);
  PRINTF("SCR  : %.2x\n", uart->regs.scr);

  PRINTF("\nInternal registers (sim debug):\n");
  PRINTF("RXSER: %.2"PRIx16"  TXSER: %.2"PRIx8"\n", uart->iregs.rxser,
         uart->iregs.txser);

  PRINTF("\nInternal status (sim debug):\n");
  PRINTF("char_clks: %ld\n", uart->char_clks);
  PRINTF("rxbuf_full: %d  txbuf_full: %d\n", uart->istat.rxbuf_full, uart->istat.txbuf_full);
  PRINTF("Using IRQ%i\n", uart->irq);
  if (uart->vapi_id)
    PRINTF ("Connected to vapi ID=%lx\n\n", uart->vapi_id);
  /* TODO: replace by a channel_status
  else
    PRINTF("RX fs: %p  TX fs: %p\n\n", uart->rxfs, uart->txfs);
  */
}

/*---------------------------------------------------[ UART configuration ]---*/
void uart_baseaddr(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->baseaddr = val.addr_val;
}

void uart_jitter(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->jitter = val.int_val;
}

void uart_irq(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->irq = val.int_val;
}

void uart_16550(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->uart16550 = val.int_val;
}

void uart_channel(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  if(!(uart->channel_str = strdup(val.str_val))) {
    fprintf(stderr, "Peripheral 16450: Run out of memory\n");
    exit(-1);
  }
}

void uart_newway(union param_val val, void *dat)
{
  CONFIG_ERROR(" txfile and rxfile and now obsolete.\n\tUse 'channel = \"file:rxfile,txfile\"' instead.");
  exit(1);
}

void uart_vapi_id(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->vapi_id = val.int_val;
}

void uart_enabled(union param_val val, void *dat)
{
  struct dev_16450 *uart = dat;
  uart->enabled = val.int_val;
}

void *uart_sec_start(void)
{
  struct dev_16450 *new = malloc(sizeof(struct dev_16450));

  if(!new) {
    fprintf(stderr, "Peripheral 16450: Run out of memory\n");
    exit(-1);
  }

  new->enabled = 1;
  new->channel_str = NULL;
  new->channel = NULL;
  new->vapi_id = 0;

  return new;
}

void uart_sec_end(void *dat)
{
  struct dev_16450 *uart = dat;
  struct mem_ops ops;

  if(!uart->enabled) {
    free(dat);
    return;
  }

  memset(&ops, 0, sizeof(struct mem_ops));

  ops.readfunc8 = uart_read_byte;
  ops.writefunc8 = uart_write_byte;
  ops.read_dat8 = dat;
  ops.write_dat8 = dat;

  /* FIXME: What should these be? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area(uart->baseaddr, UART_ADDR_SPACE, 0, &ops);

  reg_sim_reset(uart_reset, dat);
  reg_sim_stat(uart_status, dat);
}

void reg_uart_sec(void)
{
  struct config_section *sec = reg_config_sec("uart", uart_sec_start,
                                              uart_sec_end);

  reg_config_param(sec, "baseaddr", paramt_addr, uart_baseaddr);
  reg_config_param(sec, "enabled", paramt_int, uart_enabled);
  reg_config_param(sec, "irq", paramt_int, uart_irq);
  reg_config_param(sec, "16550", paramt_int, uart_16550);
  reg_config_param(sec, "jitter", paramt_int, uart_jitter);
  reg_config_param(sec, "channel", paramt_str, uart_channel);
  reg_config_param(sec, "txfile", paramt_str, uart_newway);
  reg_config_param(sec, "rxfile", paramt_str, uart_newway);
  reg_config_param(sec, "vapi_id", paramt_int, uart_vapi_id);
}
