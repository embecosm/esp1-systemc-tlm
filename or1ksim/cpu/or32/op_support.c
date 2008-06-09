/* op_support.c -- Support routines for micro operations
   Copyright (C) 2005 György `nog' Jeney, nog@sdf.lonestar.org

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

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "opcode/or32.h"
#include "sim-config.h"
#include "spr_defs.h"
#include "except.h"
#include "immu.h"
#include "abstract.h"
#include "execute.h"
#include "sched.h"

#include "i386_regs.h"

#include "dyn_rec.h"
#include "op_support.h"

#include "rec_i386.h"

/* Stuff that is really a `micro' operation but is rather big (or for some other
 * reason like calling exit()) */

void upd_reg_from_t(oraddr_t pc, int bound)
{
  int reg;

  pc = ((pc & (PAGE_SIZE - 1)) / 4);

  if(bound) {
    reg = cpu_state.curr_page->ts_bound[pc + 1];
  } else
    reg = cpu_state.curr_page->ts_during[pc];

  if(reg & 0x1f)
    cpu_state.reg[reg & 0x1f] = cpu_state.t0;

  if((reg >> 5) & 0x1f)
    cpu_state.reg[(reg >> 5) & 0x1f] = cpu_state.t1;

  if((reg >> 10) & 0x1f)
    cpu_state.reg[(reg >> 10) & 0x1f] = cpu_state.t2;
}

void op_support_nop_exit(void)
{
  upd_reg_from_t(get_pc(), 0);
  PRINTF("exit(%"PRIdREG")\n", cpu_state.reg[3]);
  fprintf(stderr, "@reset : cycles %lld, insn #%lld\n",
          runtime.sim.reset_cycles, runtime.cpu.reset_instructions);
  fprintf(stderr, "@exit  : cycles %lld, insn #%lld\n", runtime.sim.cycles,
          runtime.cpu.instructions);
  fprintf(stderr, " diff  : cycles %lld, insn #%lld\n",
          runtime.sim.cycles - runtime.sim.reset_cycles,
          runtime.cpu.instructions - runtime.cpu.reset_instructions);
  /* FIXME: Implement emulation of a stalled cpu
  if (config.debug.gdb_enabled)
    set_stall_state (1);
  else {
    handle_sim_command();
    sim_done();
  }
  */
  exit(0);
}

void op_support_nop_reset(void)
{
  PRINTF("****************** counters reset ******************\n");
  PRINTF("cycles %lld, insn #%lld\n", runtime.sim.cycles, runtime.cpu.instructions);
  PRINTF("****************** counters reset ******************\n");
  runtime.sim.reset_cycles = runtime.sim.cycles;
  runtime.cpu.reset_instructions = runtime.cpu.instructions;
}

void op_support_nop_printf(void)
{
  upd_reg_from_t(get_pc(), 0);
  simprintf(cpu_state.reg[4], cpu_state.reg[3]);
}

void op_support_nop_report(void)
{
  upd_reg_from_t(get_pc(), 0);
  PRINTF("report(0x%"PRIxREG");\n", cpu_state.reg[3]);
}

void op_support_nop_report_imm(int imm)
{
  upd_reg_from_t(get_pc(), 0);
  PRINTF("report %i (0x%"PRIxREG");\n", imm, cpu_state.reg[3]);
}

/* Handles a jump */
/* addr is a VIRTUAL address */
/* NOTE: We can't use env since this code is compiled like the rest of the
 * simulator (most likely without -fomit-frame-pointer) and thus env will point
 * to some bogus value. */
void do_jump(oraddr_t addr)
{
  struct dyn_page *target_dp;
  oraddr_t phys_page;

  /* Temporaries are always shipped out */
  cpu_state.ts_current = 1;

  /* The pc is set to the location of the jump in op_set_pc_preemt(_check) and
   * then it is incermented by 4 when the scheduler is run.  If a scheduled job
   * so happens to raise an exception cpu_state.delay_insn will still be set and
   * so except_handle will do its pc adjusting magic (ie. -4 from it) and every-
   * thing ends up just working right, except when a scheduled job does not
   * raise an exeception.  In that case we set the pc here explicitly */
  set_pc(addr);

  /* immu_translate must be called after set_pc.  If it would be called before
   * it and it issued an ITLB miss then it would appear that the instruction
   * that faulted was the instruction in the delay slot which is incorrect */
  phys_page = immu_translate(addr);

  /* do_jump is called from the delay slot, which is the jump instruction
   * address + 4. */
/*
  printf("Recompiled code jumping out to %"PRIxADDR" from %"PRIxADDR"\n",
         phys_page, cpu_state.sprs[SPR_PPC] - 4);
*/

  /* immu_translate() adds the hit delay to runtime.sim.mem_cycles but we add it
   * to the cycles when the instruction is executed so if we don't reset it now
   * it will produce wrong results */
  runtime.sim.mem_cycles = 0;

  target_dp = cpu_state.dyn_pages[phys_page >> config.immu.pagesize_log2];

  if(!target_dp)
    target_dp = new_dp(phys_page);

  /* Since writes to the 0x0-0xff range do not dirtyfy a page recompile the 0x0
   * page if the jump is to that location */
  if(phys_page < 0x100)
    target_dp->dirty = 1;

  if(target_dp->dirty)
    recompile_page(target_dp);

  cpu_state.curr_page = target_dp;

  /* FIXME: If the page is backed by more than one type of memory, this will
   * produce wrong results */
  if(cpu_state.sprs[SPR_SR] & SPR_SR_IME)
    /* Add the mmu hit delay to the cycle counter */
    upd_cycles_dec(target_dp->delayr - config.immu.hitdelay);
  else
    upd_cycles_dec(target_dp->delayr);

  cpu_state.ts_current = 0;

  /* Initially this returned the address that we should jump to and then the
   * recompiled code performed the jump.  This was no problem if the jump was
   * trully an interpage jump or if the location didn't need recompileation.  If
   * the jump is page local and the page needs recompileation there is a very
   * high probability that the page will move in memory and then the return
   * address that is on the stack will point to memory that has already been
   * freed, sometimes leading to crashes */
  /* This looks like it could really be simpler, but no it can't.  The only
   * issue here is the stack: it has to be unwound.  This function is called
   * from except_handle, which generally ends up quite high on the stack... */
  enter_dyn_code(phys_page, target_dp);
}

/* Wrapper around analysis() that contains all the recompiler specific stuff */
void op_support_analysis(void)
{
  upd_sim_cycles();
  if(IADDR_PAGE(cpu_state.pc) != cpu_state.pc)
    upd_reg_from_t(cpu_state.pc - (cpu_state.delay_insn ? 4 : 0), 0);
  else
    upd_reg_from_t(cpu_state.pc, 0);
  runtime.cpu.instructions++;
  analysis(&cpu_state.iqueue);
}

