/* icache_model.h -- instruction cache header file
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

#define MAX_IC_SETS 1024
#define MAX_IC_WAYS 32
#define MAX_IC_BLOCK_SIZE 4 /* In words */
   
uint32_t ic_simulate_fetch(oraddr_t fetchaddr, oraddr_t virt_addr);
void ic_inv(oraddr_t dataaddr);
void ic_info();
