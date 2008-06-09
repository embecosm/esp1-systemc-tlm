/* trace.c -- Simulator breakpoints
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

#include <stdio.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "labels.h"
#include "sim-config.h"
#include "trace.h"

/* Set instruction execution breakpoint. */

void set_insnbrkpoint(oraddr_t addr)
{
  addr &= ~ADDR_C(3);	/* 32-bit aligned */	
	
  if (!verify_memoryarea(addr))
    PRINTF("WARNING: This breakpoint is out of the simulated memory range.\n");

  if (has_breakpoint (addr)) {
    remove_breakpoint (addr);
    PRINTF("\nBreakpoint at 0x%"PRIxADDR" cleared.\n", addr);
  } else {
    add_breakpoint (addr);
    PRINTF("\nBreakpoint at 0x%"PRIxADDR" set.\n", addr);
  }

  return;
}

