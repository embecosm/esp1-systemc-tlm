/* vga.h -- Definition of types and structures for Richard's VGA/LCD controler.
   Copyright (C) 2002 Marko Mlinar, markom@opencores.org

NOTE: device is only partially implemented!

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

#ifndef _VGA_H_
#define _VGA_H_

#define VGA_CTRL        0x00       /* Control Register */
#define VGA_STAT        0x04       /* Status Register */
#define VGA_HTIM        0x08       /* Horizontal Timing Register */
#define VGA_VTIM        0x0c       /* Vertical Timing Register */
#define VGA_HVLEN       0x10       /* Horizontal and Vertical Length Register */
#define VGA_VBARA       0x14       /* Video Memory Base Address Register A */
#define VGA_VBARB       0x18       /* Video Memory Base Address Register B */
#define VGA_CLUTA       0x800
#define VGA_CLUTB       0xc00
#define VGA_MASK        0xfff
#define VGA_ADDR_SPACE  1024    


/* List of implemented registers; all other are ignored.  */

/* Control Register */
#define VGA_CTRL_VEN    0x00000001 /* Video Enable */
#define VGA_CTRL_CD     0x00000300 /* Color Depth */
#define VGA_CTRL_PC     0x00000400 /* Pseudo Color */

/* Status Register */
/* Horiz. Timing Register */
/* Vert. Timing Register */
/* Horiz. and Vert. Length Register */

#endif /* _VGA_H_ */
