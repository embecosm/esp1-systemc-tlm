/* sprs.c -- Simulation of OR1K special-purpose registers
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
#include <errno.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "except.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "dcache_model.h"
#include "icache_model.h"
#include "tick.h"
#include "dmmu.h"
#include "immu.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(spr);
DECLARE_DEBUG_CHANNEL(immu);

static int audio_cnt = 0;

static FILE *fo = 0;

/* Set a specific SPR with a value. */
void mtspr(uint16_t regno, const uorreg_t value)
{
  uorreg_t prev_val;

  prev_val = cpu_state.sprs[regno];
  cpu_state.sprs[regno] = value;

  TRACE("%s\n", dump_spr(regno, value));
  
  /* MM: Register hooks.  */
  switch (regno) {
  case SPR_TTCR:
    spr_write_ttcr (value);
    break;
  case SPR_TTMR:
    spr_write_ttmr (prev_val);
    break;
  /* Data cache simulateing stuff */
  case SPR_DCBPR:
    /* FIXME: This is not correct.  The arch. manual states: "Memory accesses
     * are not recorded (Unlike load or store instructions) and cannot invoke
     * any exception".  If the physical address is invalid a bus error will be
     * generated.  Also if the effective address is not resident in the mmu
     * the read will happen from address 0, which is naturally not correct. */
    dc_simulate_read(peek_into_dtlb(value, 0, 1), value, 4);
    cpu_state.sprs[SPR_DCBPR] = 0;
    break;
  case SPR_DCBFR:
    dc_inv(value);
    cpu_state.sprs[SPR_DCBFR] = -1;
    break;
  case SPR_DCBIR:
    dc_inv(value);
    cpu_state.sprs[SPR_DCBIR] = 0;
    break;
  case SPR_DCBWR:
    cpu_state.sprs[SPR_DCBWR] = 0;
    break;
  case SPR_DCBLR:
    cpu_state.sprs[SPR_DCBLR] = 0;
    break;
  /* Instruction cache simulateing stuff */
  case SPR_ICBPR:
    /* FIXME: The arch manual does not say what happens when an invalid memory
     * location is specified.  I guess the same as for the DCBPR register */
    ic_simulate_fetch(peek_into_itlb(value), value);
    cpu_state.sprs[SPR_ICBPR] = 0;
    break;
  case SPR_ICBIR:
    ic_inv(value);
    cpu_state.sprs[SPR_ICBIR] = 0;
    break;
  case SPR_ICBLR:
    cpu_state.sprs[SPR_ICBLR] = 0;
    break;
  case SPR_SR:
    cpu_state.sprs[regno] |= SPR_SR_FO;
#if DYNAMIC_EXECUTION
    if((value & SPR_SR_IME) && !(prev_val & SPR_SR_IME)) {
      TRACE_(immu)("IMMU just became enabled (%lli).\n", runtime.sim.cycles);
      recheck_immu(IMMU_GOT_ENABLED);
    } else if(!(value & SPR_SR_IME) && (prev_val & SPR_SR_IME)) {
      TRACE_(immu)("Remove counting of mmu hit delay with cycles (%lli)\n",
                   runtime.sim.cycles);
      recheck_immu(IMMU_GOT_DISABLED);
    }
#endif
    break;
  case SPR_NPC:
    {
      /* The debugger has redirected us to a new address */
      /* This is usually done to reissue an instruction
         which just caused a breakpoint exception. */
      cpu_state.pc = value;
    
      if(!value && config.sim.verbose)
        PRINTF("WARNING: PC just set to 0!\n");
    
      /* Clear any pending delay slot jumps also */
      cpu_state.delay_insn = 0;
      pcnext = value + 4;
    }
    break;
  case 0xFFFD:
    fo = fopen ("audiosim.pcm", "wb+");
    if (!fo) PRINTF("Cannot open audiosim.pcm\n");
    PRINTF("Audio opened.\n");
    break;
  case 0xFFFE:
    if (!fo) PRINTF("audiosim.pcm not opened\n");
    fputc (value & 0xFF, fo);
    if ((audio_cnt % 1024) == 0)
      PRINTF("%i\n", audio_cnt);
    audio_cnt++;
    break;
  case 0xFFFF:
    fclose(fo);
    PRINTF("Audio closed.\n");
    sim_done();
    break;
  case SPR_PMR:
    /* PMR[SDF] and PMR[DCGE] are ignored completely. */
    if (value & SPR_PMR_SUME) {
      PRINTF ("SUSPEND: PMR[SUME] bit was set.\n");
      sim_done();
    }
    break;
  default:
    /* Mask reserved bits in DTLBMR and DTLBMR registers */
    if ( (regno >= SPR_DTLBMR_BASE(0)) && (regno < SPR_DTLBTR_LAST(3))) {
      if((regno & 0xff) < 0x80)
        cpu_state.sprs[regno] = ((value / config.dmmu.pagesize) * config.dmmu.pagesize) | 
                              (value & (SPR_DTLBMR_V | SPR_DTLBMR_PL1 | SPR_DTLBMR_CID | SPR_DTLBMR_LRU));
      else
        cpu_state.sprs[regno] = ((value / config.dmmu.pagesize) * config.dmmu.pagesize) |
                              (value & (SPR_DTLBTR_CC | SPR_DTLBTR_CI | SPR_DTLBTR_WBC | SPR_DTLBTR_WOM |
                              SPR_DTLBTR_A | SPR_DTLBTR_D | SPR_DTLBTR_URE | SPR_DTLBTR_UWE | SPR_DTLBTR_SRE |
                              SPR_DTLBTR_SWE));
    }

    /* Mask reseved bits in ITLBMR and ITLBMR registers */
    if ( (regno >= SPR_ITLBMR_BASE(0)) && (regno < SPR_ITLBTR_LAST(3))) {
      TRACE_(immu)("Writting to an mmu way (reg: %"PRIx16", value: %"PRIx32")\n",
                   regno, value);
      if((regno & 0xff) < 0x80)
        cpu_state.sprs[regno] = ((value / config.immu.pagesize) * config.immu.pagesize) | 
                              (value & (SPR_ITLBMR_V | SPR_ITLBMR_PL1 | SPR_ITLBMR_CID | SPR_ITLBMR_LRU));
      else
        cpu_state.sprs[regno] = ((value / config.immu.pagesize) * config.immu.pagesize) |
                              (value & (SPR_ITLBTR_CC | SPR_ITLBTR_CI | SPR_ITLBTR_WBC | SPR_ITLBTR_WOM |
                              SPR_ITLBTR_A | SPR_ITLBTR_D | SPR_ITLBTR_SXE | SPR_ITLBTR_UXE));

#if DYNAMIC_EXECUTION
      if(cpu_state.sprs[SPR_SR] & SPR_SR_IME) {
        /* The immu got reconfigured.  Recheck if the current page in execution
         * is resident in the immu ways.  This check would be done during the
         * instruction fetch but since the dynamic execution model does not do
         * instruction fetchs, do it now. */
        recheck_immu(0);
      }
#endif
    }

    /* Links to GPRS */
    if(regno >= 0x0400 && regno < 0x0420) {
      cpu_state.reg[regno - 0x0400] = value;
    }
    break;
  }
}

/* Get a specific SPR. */
uorreg_t mfspr(const uint16_t regno)
{
  uorreg_t ret;

  ret = cpu_state.sprs[regno];

  switch (regno) {
  case SPR_NPC:
    ret = cpu_state.pc;
    break;
  case SPR_TTCR:
    ret = spr_read_ttcr();
    break;
  default:
    /* Links to GPRS */
    if(regno >= 0x0400 && regno < 0x0420)
      ret = cpu_state.reg[regno - 0x0400];
  }

  TRACE("%s\n", dump_spr(regno, ret));

  return ret;
}

/* Show status of important SPRs. */
void sprs_status(void)
{
  PRINTF("VR   : 0x%"PRIxREG"  UPR  : 0x%"PRIxREG"\n", cpu_state.sprs[SPR_VR],
         cpu_state.sprs[SPR_UPR]);
  PRINTF("SR   : 0x%"PRIxREG"\n", cpu_state.sprs[SPR_SR]);
  PRINTF("MACLO: 0x%"PRIxREG"  MACHI: 0x%"PRIxREG"\n",
         cpu_state.sprs[SPR_MACLO], cpu_state.sprs[SPR_MACHI]);
  PRINTF("EPCR0: 0x%"PRIxADDR"  EPCR1: 0x%"PRIxADDR"\n",
         cpu_state.sprs[SPR_EPCR_BASE], cpu_state.sprs[SPR_EPCR_BASE+1]);
  PRINTF("EEAR0: 0x%"PRIxADDR"  EEAR1: 0x%"PRIxADDR"\n",
         cpu_state.sprs[SPR_EEAR_BASE], cpu_state.sprs[SPR_EEAR_BASE+1]);
  PRINTF("ESR0 : 0x%"PRIxREG"  ESR1 : 0x%"PRIxREG"\n",
         cpu_state.sprs[SPR_ESR_BASE], cpu_state.sprs[SPR_ESR_BASE+1]);
  PRINTF("TTMR : 0x%"PRIxREG"  TTCR : 0x%"PRIxREG"\n",
         cpu_state.sprs[SPR_TTMR], cpu_state.sprs[SPR_TTCR]);
  PRINTF("PICMR: 0x%"PRIxREG"  PICSR: 0x%"PRIxREG"\n",
         cpu_state.sprs[SPR_PICMR], cpu_state.sprs[SPR_PICSR]);
  PRINTF("PPC:   0x%"PRIxADDR"  NPC   : 0x%"PRIxADDR"\n",
         cpu_state.sprs[SPR_PPC], cpu_state.sprs[SPR_NPC]);
}
