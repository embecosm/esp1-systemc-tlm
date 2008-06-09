/* fb.h -- Definition of types and structures for simple frame buffer.
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

#ifndef _FB_H_
#define _FB_H_

#define FB_SIZEX           640
#define FB_SIZEY           480

#define CAM_SIZEX	   352
#define CAM_SIZEY	   288

/* Relative amount of time spent in refresh */
#define REFRESH_DIVIDER	   20

#define FB_CTRL            0x0000
#define FB_BUFADDR         0x0004
#define FB_CAMBUFADDR      0x0008
#define FB_CAMPOSADDR      0x000c
#define FB_PAL             0x0400

#endif /* _VGA_H_ */
