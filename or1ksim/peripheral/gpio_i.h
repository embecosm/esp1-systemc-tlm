/* gpio_i.h -- Definition of internal types and structures for GPIO code
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

#ifndef __OR1KSIM_PERIPHERAL_GPIO_I_H
#define __OR1KSIM_PERIPHERAL_GPIO_I_H

#include "gpio.h"
#include "config.h"

/*
 * The various VAPI IDs each GPIO device has
 */
enum { GPIO_VAPI_DATA = 0,
       GPIO_VAPI_AUX,
       GPIO_VAPI_CLOCK,
       GPIO_VAPI_RGPIO_OE,
       GPIO_VAPI_RGPIO_INTE,
       GPIO_VAPI_RGPIO_PTRIG,
       GPIO_VAPI_RGPIO_AUX,
       GPIO_VAPI_RGPIO_CTRL, 
       GPIO_NUM_VAPI_IDS };


/*
 * Implementatino of GPIO Code Registers and State
 */
struct gpio_device 
{
  /* Is peripheral enabled */
  int enabled;

  /* Base address in memory */
  oraddr_t baseaddr;

  /* Which IRQ to generate */
  int irq;

  /* Which GPIO is this? */
  unsigned gpio_number;

  /* VAPI IDs */
  unsigned long base_vapi_id;

  /* Auxiliary inputs */
  unsigned long auxiliary_inputs;

  /* Visible registers */
  struct
  {
    unsigned long in;
    unsigned long out;
    unsigned long oe;
    unsigned long inte;
    unsigned long ptrig;
    unsigned long aux;
    unsigned long ctrl;
    unsigned long ints;

    int external_clock;
  } curr, next;
};




#endif /* __OR1KSIM_PERIPHERAL_GPIO_I_H */
