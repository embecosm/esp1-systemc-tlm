/* dcache_model.h -- data cache header file
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

#define MAX_DC_SETS 1024
#define MAX_DC_WAYS 32
#define MAX_DC_BLOCK_SIZE 4 /* In words */

uint32_t dc_simulate_read(oraddr_t dataaddr, oraddr_t virt_addr, int width);
void dc_simulate_write(oraddr_t dataaddr, oraddr_t virt_addr, uint32_t data, int width);
void dc_info();
void dc_inv(oraddr_t dataaddr);
