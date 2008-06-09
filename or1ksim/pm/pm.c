/* pm.c -- Simulation of OpenRISC 1000 power management
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

/* This is functional simulation of OpenRISC 1000 architectural
   power management.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "pm.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "sim-config.h"

/* Reset. It initializes PMR register. */
void pm_reset(void)
{
	if (config.sim.verbose) PRINTF("Resetting Power Management.\n");
	cpu_state.sprs[SPR_PMR] = 0;
}

/*-----------------------------------------------------[ PM Configuration ]---*/
void pm_enabled(union param_val val, void *dat)
{
  config.pm.enabled = val.int_val;
}

void reg_pm_sec(void)
{
  struct config_section *sec = reg_config_sec("pm", NULL, NULL);

  reg_config_param(sec, "enabled", paramt_int, pm_enabled);
}
