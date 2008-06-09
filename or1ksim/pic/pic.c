/* pic.c -- Simulation of OpenRISC 1000 programmable interrupt controller
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
   programmable interrupt controller.
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
#include "pic.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "except.h"
#include "sprs.h"
#include "sim-config.h"
#include "sched.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(pic);

/* Reset. It initializes PIC registers. */
void pic_reset()
{
  PRINTF("Resetting PIC.\n");
  cpu_state.sprs[SPR_PICMR] = 0;
  cpu_state.sprs[SPR_PICPR] = 0;
  cpu_state.sprs[SPR_PICSR] = 0;
}

/* Handles the reporting of an interrupt if it had to be delayed */
void pic_clock(void *dat)
{
  /* Don't do anything if interrupts not currently enabled */
  if(cpu_state.sprs[SPR_SR] & SPR_SR_IEE) {
    TRACE("Delivering interrupt on cycle %lli\n", runtime.sim.cycles);
    except_handle(EXCEPT_INT, cpu_state.sprs[SPR_EEAR_BASE]);
  } else if(cpu_state.sprs[SPR_PICSR] & cpu_state.sprs[SPR_PICMR])
    /* Reschedule only if the interrupt hasn't been cleared */
    sched_next_insn(pic_clock, NULL);
}

/* WARNING: Don't eaven try and call this function *during* a simulated
 * instruction!! (as in during a read_mem or write_mem callback).  except_handle
 * assumes that this is the case, it breaks otherwise. */
/* WARNING2: Don't except report_interrupt to return!  However, it also _may_
 * return.  Make sure you handle this case aswell. */
/* Asserts interrupt to the PIC. */
void report_interrupt(int line)
{
  /* Disable doze and sleep mode */
  cpu_state.sprs[SPR_PMR] &= ~(SPR_PMR_DME | SPR_PMR_SME);

  TRACE("Asserting interrupt %d (%s).\n", line,
        (cpu_state.sprs[SPR_PICMR] & (1 << line)) ? "Unmasked" : "Masked");

  SCHED_FIND_REMOVE(pic_clock, NULL);

  if ((cpu_state.sprs[SPR_PICMR] & (1 << line)) || line < 2) {
    cpu_state.sprs[SPR_PICSR] |= 1 << line;
    /* Don't do anything if interrupts not currently enabled */
    if (cpu_state.sprs[SPR_SR] & SPR_SR_IEE) {
      TRACE("Delivering interrupt on cycle %lli\n", runtime.sim.cycles);
      except_handle(EXCEPT_INT, cpu_state.sprs[SPR_EEAR_BASE]);
      return;
    }
  }

  if(cpu_state.sprs[SPR_PICMR] & cpu_state.sprs[SPR_PICSR])
    /* Interrupts not currently enabled, retry next clock cycle */
    sched_next_insn(pic_clock, NULL);
}
