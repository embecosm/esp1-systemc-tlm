/* ps2kbd.h -- Very simple PS/2 keyboard simulation header file
   Copyright (C) 2002 Marko Mlinar, markom@opencores.org

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

#ifndef _PS2KBD_H_
#define _PS2KBD_H_

/* Device registers */
#define KBD_CTRL        4
#define KBD_DATA        0
#define KBD_SPACE       8

/* Keyboard commands */
#define KBD_KCMD_RST	0xFF
#define KBD_KCMD_DK	0xF5
#define KBD_KCMD_EK	0xF4
#define KBD_KCMD_ECHO	0xFF
#define KBD_KCMD_SRL	0xED

/* Keyboard responses */
#define KBD_KRESP_RSTOK	0xAA
#define KBD_KRESP_ECHO	0xEE
#define KBD_KRESP_ACK	0xFA

/* Controller commands */
#define KBD_CCMD_RCB	0x20
#define KBD_CCMD_WCB	0x60
#define KBD_CCMD_ST1	0xAA
#define KBD_CCMD_ST2	0xAB
#define KBD_CCMD_DKI	0xAD
#define KBD_CCMD_EKI	0xAE

/* Status register bits */
#define KBD_STATUS_OBF	0x01
#define KBD_STATUS_IBF	0x02
#define KBD_STATUS_SYS	0x04
#define KBD_STATUS_A2	0x08
#define KBD_STATUS_INH	0x10
#define KBD_STATUS_MOBF	0x20
#define KBD_STATUS_TO	0x40
#define KBD_STATUS_PERR	0x80

/* Command byte register bits */
#define KBD_CCMDBYTE_INT	0x01
#define KBD_CCMDBYTE_INT2 	0x02
#define KBD_CCMDBYTE_SYS	0x04
#define KBD_CCMDBYTE_EN		0x10
#define KBD_CCMDBYTE_EN2	0x20
#define KBD_CCMDBYTE_XLAT 	0x40

/* Length of internal scan code fifo */
#define KBD_MAX_BUF     0x100

/* Keyboard is checked every KBD_SLOWDOWN cycle */
#define KBD_BAUD_RATE   1200

#endif /* !_PS2KBD_H_ */
