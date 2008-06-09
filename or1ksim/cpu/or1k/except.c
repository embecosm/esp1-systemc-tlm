/* except.c -- Simulation of OR1K exceptions
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
#include "except.h"
#include "sim-config.h"
#include "debug_unit.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "debug.h"

#if DYNAMIC_EXECUTION
#include "sched.h"
#include "rec_i386.h"
#include "op_support.h"
#endif

DEFAULT_DEBUG_CHANNEL(except);

int except_pending = 0;

static const char *except_names[] = {
 NULL,
 "Reset",
 "Bus Error",
 "Data Page Fault",
 "Insn Page Fault",
 "Tick timer",
 "Alignment",
 "Illegal instruction",
 "Interrupt",
 "Data TLB Miss",
 "Insn TLB Miss",
 "Range",
 "System Call",
 "Floating Point",
 "Trap" };

const char *except_name(oraddr_t except)
{
  return except_names[except >> 8];
}

#if DYNAMIC_EXECUTION
/* FIXME: Remove the need for this */
/* This is needed because immu_translate can be called from do_rfe and do_jump
 * in which case the scheduler does not need to get run. immu_translate can also
 * be called from mtspr in which case the exceptions that it generates happen
 * during an instruction and the scheduler needs to get run. */
int immu_ex_from_insn = 0;
#endif

/* Asserts OR1K exception. */
/* WARNING: Don't excpect except_handle to return.  Sometimes it _may_ return at
 * other times it may not. */
void except_handle(oraddr_t except, oraddr_t ea)
{
  oraddr_t except_vector;

  if(debug_ignore_exception (except))
    return;

#if !(DYNAMIC_EXECUTION)
  /* In the dynamic recompiler, this function never returns, so this is not
   * needed.  Ofcourse we could set it anyway, but then all code that checks
   * this variable would break, since it is never reset */
  except_pending = 1;
#endif

  TRACE("Exception 0x%"PRIxADDR" (%s) at 0x%"PRIxADDR", EA: 0x%"PRIxADDR
        ", cycles %lld, #%lld\n",
        except, except_name(except), cpu_state.pc, ea, runtime.sim.cycles,
        runtime.cpu.instructions);

  except_vector = except + (cpu_state.sprs[SPR_SR] & SPR_SR_EPH ? 0xf0000000 : 0x00000000);

#if !(DYNAMIC_EXECUTION)
  pcnext = except_vector;
#endif

  cpu_state.sprs[SPR_EEAR_BASE] =  ea;
  cpu_state.sprs[SPR_ESR_BASE] = cpu_state.sprs[SPR_SR];

  cpu_state.sprs[SPR_SR] &= ~SPR_SR_OVE;   /* Disable overflow flag exception. */

  cpu_state.sprs[SPR_SR] |= SPR_SR_SM;    /* SUPV mode */
  cpu_state.sprs[SPR_SR] &= ~(SPR_SR_IEE | SPR_SR_TEE);   /* Disable interrupts. */

  /* Address translation is always disabled when starting exception. */
  cpu_state.sprs[SPR_SR] &= ~SPR_SR_DME;

#if DYNAMIC_EXECUTION
  /* If we were called from do_scheduler and there were more jobs scheduled to
   * run after this, they won't run unless the following call is made since this
   * function never returns.  (If we weren't called from do_scheduler, then the
   * job at the head of the queue will still have some time remaining) */
  if(scheduler.job_queue->time <= 0)
    do_scheduler();
#endif

  switch(except) {
  /* EPCR is irrelevent */
  case EXCEPT_RESET:
    break;
  /* EPCR is loaded with address of instruction that caused the exception */
  case EXCEPT_ITLBMISS:
  case EXCEPT_IPF:
#if DYNAMIC_EXECUTION
    /* In immu_translate except_handle is called with except_handle(..., virtaddr) */
    /* Add the immu miss delay to the cycle counter */
    if(!immu_ex_from_insn) {
      cpu_state.sprs[SPR_EPCR_BASE] = get_pc() - (cpu_state.delay_insn ? 4 : 0);
    } else
      /* This exception came from an l.mtspr instruction in which case the pc
       * points to the l.mtspr instruction when in acutal fact, it is the next
       * instruction that would have faulted/missed.  ea is used instead of
       * cpu_state.pc + 4 because in the event that the l.mtspr instruction is
       * in the delay slot of a page local jump the fault must happen on the
       * instruction that was jumped to.  This is handled in recheck_immu. */
      cpu_state.sprs[SPR_EPCR_BASE] = ea;
    run_sched_out_of_line(immu_ex_from_insn);
    /* Save the registers that are in the temporaries */
    if(!cpu_state.ts_current)
      upd_reg_from_t(cpu_state.pc, !immu_ex_from_insn);
    immu_ex_from_insn = 0;
    break;
#endif
  /* All these exceptions happen during a simulated instruction */
  case EXCEPT_BUSERR:
  case EXCEPT_DPF:
  case EXCEPT_ALIGN:
  case EXCEPT_ILLEGAL:
  case EXCEPT_DTLBMISS:
  case EXCEPT_RANGE:
  case EXCEPT_TRAP:
#if DYNAMIC_EXECUTION
    /* Since these exceptions happen during a simulated instruction and this
     * function jumps out to the exception vector the scheduler would never have
     * a chance to run, therefore run it now */
    run_sched_out_of_line(1);
    /* Save the registers that are in the temporaries */
    if(!cpu_state.ts_current) {
      if(cpu_state.delay_insn &&
         (IADDR_PAGE(cpu_state.pc) == IADDR_PAGE(cpu_state.pc - 4)))
        upd_reg_from_t(cpu_state.pc - 4, 0);
      else
        upd_reg_from_t(cpu_state.pc, 0);
    }
#endif
    cpu_state.sprs[SPR_EPCR_BASE] = cpu_state.pc - (cpu_state.delay_insn ? 4 : 0);
    break;
  /* EPCR is loaded with address of next not-yet-executed instruction */
  case EXCEPT_SYSCALL:
    cpu_state.sprs[SPR_EPCR_BASE] = (cpu_state.pc + 4) - (cpu_state.delay_insn ? 4 : 0);
    break;
  /* These exceptions happen AFTER (or before) an instruction has been
   * simulated, therefore the pc already points to the *next* instruction */
  case EXCEPT_TICK:
  case EXCEPT_INT:
    cpu_state.sprs[SPR_EPCR_BASE] = cpu_state.pc - (cpu_state.delay_insn ? 4 : 0);
#if !(DYNAMIC_EXECUTION)
    /* If we don't update the pc now, then it will only happen *after* the next
     * instruction (There would be serious problems if the next instruction just
     * happens to be a branch), when it should happen NOW. */
    cpu_state.pc = pcnext;
    pcnext += 4;
#else
    /* except_handle() mucks around with the temporaries, which are in the state
     * of the last instruction executed and not the next one, to which the pc
     * now points to */
    cpu_state.pc -= 4;

    /* Save the registers that are in the temporaries */
    if(!cpu_state.ts_current)
      upd_reg_from_t(cpu_state.pc, 1);
#endif
    break;
  }

  /* Address trnaslation is here because run_sched_out_of_line calls
   * eval_insn_direct which checks out the immu for the address translation but
   * if it would be disabled above then there would be not much point... */
  cpu_state.sprs[SPR_SR] &= ~SPR_SR_IME;

  /* Complex/simple execution strictly don't need this because of the
   * next_delay_insn thingy but in the dynamic execution modell that doesn't
   * exist and thus cpu_state.delay_insn would stick in the exception handler
   * causeing grief if the first instruction of the exception handler is also in
   * the delay slot of the previous instruction */
  cpu_state.delay_insn = 0;

#if DYNAMIC_EXECUTION
  cpu_state.pc = except_vector;
  cpu_state.ts_current = 0;
  jump_dyn_code(except_vector);
#endif
}
