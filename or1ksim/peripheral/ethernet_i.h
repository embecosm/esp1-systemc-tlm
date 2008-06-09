/* ethernet_i.h -- Definition of internal types and structures for Ethernet MAC
   Copyright (C) 2001 Erez Volk, erez@mailandnews.comopencores.org

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __OR1KSIM_PERIPHERAL_ETHERNET_I_H
#define __OR1KSIM_PERIPHERAL_ETHERNET_I_H

#include "ethernet.h"
#include "config.h"

#if HAVE_ETH_PHY
#include <netpacket/packet.h>
#endif /* HAVE_ETH_PHY */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

/*
 * Ethernet protocol definitions
 */
#if HAVE_NET_ETHERNET_H
# include <net/ethernet.h>
#elif HAVE_SYS_ETHERNET_H
# include <sys/ethernet.h>
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN ETHERADDRL
#endif
#ifndef ETHER_HDR_LEN
#define ETHER_HDR_LEN sizeof(struct ether_header)
#endif
#else /* !HAVE_NET_ETHERNET_H && !HAVE_SYS_ETHERNET_H -*/

#include <sys/types.h>

#ifdef __CYGWIN__
/* define some missing cygwin defines.
 *
 * NOTE! there is no nonblocking socket option implemented in cygwin.dll
 *       so defining MSG_DONTWAIT is just (temporary) workaround !!!
 */
#define MSG_DONTWAIT  0x40
#define ETH_HLEN      14
#endif /* __CYGWIN__ */

#define ETH_ALEN    6

struct ether_addr
{
  u_int8_t ether_addr_octet[ETH_ALEN];
};

struct ether_header
{
  u_int8_t  ether_dhost[ETH_ALEN];	/* destination eth addr	*/
  u_int8_t  ether_shost[ETH_ALEN];	/* source ether addr	*/
  u_int16_t ether_type;		        /* packet type ID field	*/
};

/* Ethernet protocol ID's */
#define	ETHERTYPE_PUP		0x0200          /* Xerox PUP */
#define	ETHERTYPE_IP		0x0800		/* IP */
#define	ETHERTYPE_ARP		0x0806		/* Address resolution */
#define	ETHERTYPE_REVARP	0x8035		/* Reverse ARP */

#define	ETHER_ADDR_LEN	ETH_ALEN                 /* size of ethernet addr */
#define	ETHER_TYPE_LEN	2                        /* bytes in type field */
#define	ETHER_CRC_LEN	4                        /* bytes in CRC field */
#define	ETHER_HDR_LEN	ETH_HLEN                 /* total octets in header */
#define	ETHER_MIN_LEN	(ETH_ZLEN + ETHER_CRC_LEN) /* min packet length */
#define	ETHER_MAX_LEN	(ETH_FRAME_LEN + ETHER_CRC_LEN) /* max packet length */

/* make sure ethenet length is valid */
#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	ETH_DATA_LEN
#define	ETHERMIN	(ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)

#endif /* HAVE_NET_ETHERNET_H */


/*
 * Implementatino of Ethernet MAC Registers and State
 */
#define ETH_TXSTATE_IDLE	0
#define ETH_TXSTATE_WAIT4BD	10
#define ETH_TXSTATE_READFIFO	20
#define ETH_TXSTATE_TRANSMIT	30

#define ETH_RXSTATE_IDLE	0
#define ETH_RXSTATE_WAIT4BD	10
#define ETH_RXSTATE_RECV	20
#define ETH_RXSTATE_WRITEFIFO	30

#define ETH_RTX_FILE    0
#define ETH_RTX_SOCK    1
#define ETH_RTX_VAPI	2

#define ETH_MAXPL   0x10000

enum { ETH_VAPI_DATA = 0,
       ETH_VAPI_CTRL,
       ETH_NUM_VAPI_IDS };

struct eth_device 
{
  /* Is peripheral enabled */
  int enabled;

  /* Base address in memory */
  oraddr_t baseaddr;

  /* Which DMA controller is this MAC connected to */
  unsigned dma;
	unsigned tx_channel;
	unsigned rx_channel;

  /* Our address */
  unsigned char mac_address[ETHER_ADDR_LEN];
  
  /* interrupt line */
  unsigned long mac_int;

  /* VAPI ID */
  unsigned long base_vapi_id;

  /* RX and TX file names and handles */
  char *rxfile, *txfile;
	int txfd;
	int rxfd;
	off_t loopback_offset;

  /* Socket interface name */
  char *sockif;

    int rtx_sock;
    int rtx_type;
    struct ifreq ifr;
    fd_set rfds, wfds;
    
	/* Current TX state */
	struct 
	{
	    unsigned long state;
	    unsigned long bd_index;
	    unsigned long bd;
	    unsigned long bd_addr;
	    unsigned working, waiting_for_dma, error;
	    long packet_length;
	    unsigned minimum_length, maximum_length;
	    unsigned add_crc;
	    unsigned crc_dly;
	    unsigned long crc_value;
	    long bytes_left, bytes_sent;
	} tx;

	/* Current RX state */
	struct 
	{
	    unsigned long state;
	    unsigned long bd_index;
	    unsigned long bd;
	    unsigned long bd_addr;
	    int fd;
	    off_t *offset;
	    unsigned working, error, waiting_for_dma;
	    long packet_length, bytes_read, bytes_left;
	} rx;

  /* Visible registers */
  struct
  {
    unsigned long moder;
    unsigned long int_source;
    unsigned long int_mask;
    unsigned long ipgt;
    unsigned long ipgr1;
    unsigned long ipgr2;
    unsigned long packetlen;
    unsigned long collconf;
    unsigned long tx_bd_num;
    unsigned long controlmoder;
    unsigned long miimoder;
    unsigned long miicommand;
    unsigned long miiaddress;
    unsigned long miitx_data;
    unsigned long miirx_data;
    unsigned long miistatus;
    unsigned long hash0;
    unsigned long hash1;
		
    /* Buffer descriptors */
    unsigned long bd_ram[ETH_BD_SPACE / 4];
  } regs;

    unsigned char rx_buff[ETH_MAXPL];
    unsigned char tx_buff[ETH_MAXPL];
    unsigned char lo_buff[ETH_MAXPL];
};

#endif /* __OR1KSIM_PERIPHERAL_ETHERNET_I_H */
