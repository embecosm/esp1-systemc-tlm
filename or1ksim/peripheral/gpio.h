/* gpio.h -- Definition of types and structures for the GPIO code
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

#ifndef __OR1KSIM_PERIPHERAL_GPIO_H
#define __OR1KSIM_PERIPHERAL_GPIO_H

/* Address space required by one GPIO */
#define GPIO_ADDR_SPACE 0x20

/* Relative Register Addresses */
#define RGPIO_IN        0x00
#define RGPIO_OUT       0x04
#define RGPIO_OE        0x08
#define RGPIO_INTE      0x0C
#define RGPIO_PTRIG     0x10
#define RGPIO_AUX       0x14
#define RGPIO_CTRL      0x18
#define RGPIO_INTS      0x1C

/* Fields inside RGPIO_CTRL */
#define RGPIO_CTRL_ECLK      0x00000001
#define RGPIO_CTRL_NEC       0x00000002
#define RGPIO_CTRL_INTE      0x00000004
#define RGPIO_CTRL_INTS      0x00000008


#endif /* __OR1KSIM_PERIPHERAL_GPIO_H */
