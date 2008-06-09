/* dmmu.h -- Data MMU header file
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
   
oraddr_t dmmu_translate(oraddr_t virtaddr, int write_access);
oraddr_t dmmu_simulate_tlb(oraddr_t virtaddr, int write_access);
oraddr_t peek_into_dtlb(oraddr_t virtaddr, int write_access, int through_dc);
void dtlb_status(int start_set);
void init_dmmu(void);
