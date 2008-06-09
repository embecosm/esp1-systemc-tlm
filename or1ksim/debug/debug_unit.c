/* debug_unit.c -- Simulation of Or1k debug unit
   Copyright (C) 2001 Chris Ziomkowski, chris@asics.ws

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

/*
  This is an architectural level simulation of the Or1k debug
  unit as described in OpenRISC 1000 System Architecture Manual,
  v. 0.1 on 22 April, 2001. This unit is described in Section 13.

  Every attempt has been made to be as accurate as possible with
  respect to the registers and the behavior. There are no known
  limitations at this time.
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
#include "debug_unit.h"
#include "sim-config.h"
#include "except.h"
#include "abstract.h"
#include "parse.h"
#include "gdb.h"
#include "except.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "debug.h"

DECLARE_DEBUG_CHANNEL(jtag);

DevelopmentInterface development;

/* External STALL signal to debug interface */
int in_reset = 0;

/* Current watchpoint state */
unsigned long watchpoints = 0;

static int calculate_watchpoints(DebugUnitAction action, unsigned long udata);

void set_stall_state(int state)
{
#if DYNAMIC_EXECUTION
  if(state)
    PRINTF("FIXME: Emulating a stalled cpu not implemented (in the dynamic execution model)\n");
#endif
  development.riscop &= ~RISCOP_STALL;
  development.riscop |= state ? RISCOP_STALL : 0;
  if(cpu_state.sprs[SPR_DMR1] & SPR_DMR1_DXFW) /* If debugger disabled */
    state = 0;
  runtime.cpu.stalled = state;
}

void du_reset(void)
{
  development.riscop = 0;
  set_stall_state (0);
}

void du_clock(void)
{
  watchpoints=0;
};

int CheckDebugUnit(DebugUnitAction action, unsigned long udata)
{
  /* Do not stop, if we have debug module disabled or during reset */
  if(!config.debug.enabled || in_reset)
    return 0;
  
  /* If we're single stepping, always stop */
  if((action == DebugInstructionFetch) && (cpu_state.sprs[SPR_DMR1] & SPR_DMR1_ST))
    return 1;

  /* is any watchpoint enabled to generate a break or count? If not, ignore */
  if(cpu_state.sprs[SPR_DMR2] & (SPR_DMR2_WGB | SPR_DMR2_AWTC))
    return calculate_watchpoints(action, udata);

  return 0;
}

/* Checks whether we should stall the RISC or cause an exception */
static int calculate_watchpoints(DebugUnitAction action, unsigned long udata)
{
  int breakpoint = 0;
  int i, bit;

  /* Hopefully this loop would be unrolled run at max. speed */
  for(i = 0, bit = 1; i < 11; i++, bit <<= 1) {
    int chain1, chain2;
    int match = 0;
    int DCR_hit = 0;

    /* Calculate first 8 matchpoints, result is put into DCR_hit */
    if (i < 8) {
      unsigned long dcr = cpu_state.sprs[SPR_DCR(i)];
      unsigned long dcr_ct = dcr & SPR_DCR_CT; /* the CT field alone */
      /* Is this matchpoint a propos for the current action? */
      if ( ((dcr & SPR_DCR_DP) && dcr_ct) && /* DVR/DCP pair present */
            (((action==DebugInstructionFetch) && (dcr_ct == SPR_DCR_CT_IFEA)) ||
           ((action==DebugLoadAddress) && ((dcr_ct == SPR_DCR_CT_LEA) ||
                                           (dcr_ct == SPR_DCR_CT_LSEA))) ||
           ((action==DebugStoreAddress) && ((dcr_ct == SPR_DCR_CT_SEA) ||
                                            (dcr_ct == SPR_DCR_CT_LSEA))) ||
           ((action==DebugLoadData) && ((dcr_ct == SPR_DCR_CT_LD) ||
                                        (dcr_ct == SPR_DCR_CT_LSD))) ||
           ((action==DebugStoreData) && ((dcr_ct == SPR_DCR_CT_SD) ||
                                         (dcr_ct == SPR_DCR_CT_LSD)))) ) {
        unsigned long op1 = udata;
        unsigned long op2 = cpu_state.sprs[SPR_DVR(i)];
        /* Perform signed comparison?  */
        if (dcr & SPR_DCR_SC) {
          long sop1 = op1, sop2 = op2; /* Convert to signed */
          switch(dcr & SPR_DCR_CC) {
          case SPR_DCR_CC_MASKED: DCR_hit = sop1 & sop2; break;
          case SPR_DCR_CC_EQUAL: DCR_hit = sop1 == sop2; break;
          case SPR_DCR_CC_NEQUAL: DCR_hit = sop1 != sop2; break;
          case SPR_DCR_CC_LESS: DCR_hit = sop1 < sop2; break;
          case SPR_DCR_CC_LESSE: DCR_hit = sop1 <= sop2; break;
          case SPR_DCR_CC_GREAT: DCR_hit = sop1 > sop2; break;
          case SPR_DCR_CC_GREATE: DCR_hit = sop1 >= sop2; break;
          }
        } else {
          switch(dcr & SPR_DCR_CC) {
          case SPR_DCR_CC_MASKED: DCR_hit = op1 & op2; break;
          case SPR_DCR_CC_EQUAL: DCR_hit = op1 == op2; break;
          case SPR_DCR_CC_NEQUAL: DCR_hit = op1 != op2; break;
          case SPR_DCR_CC_LESS: DCR_hit = op1 < op2; break;
          case SPR_DCR_CC_LESSE: DCR_hit = op1 <= op2; break;
          case SPR_DCR_CC_GREAT: DCR_hit = op1 > op2; break;
          case SPR_DCR_CC_GREATE: DCR_hit = op1 >= op2; break;
          }
        } 
      }
    }

    /* Chain matchpoints */
    switch(i) {
    case 0:
      chain1 = chain2 = DCR_hit;
      break;
    case 8:
      chain1 = (cpu_state.sprs[SPR_DWCR0] & SPR_DWCR_COUNT) ==
               (cpu_state.sprs[SPR_DWCR0] & SPR_DWCR_MATCH);
      chain2 = watchpoints & (1 << 7);
      break;
    case 9:
      chain1 = (cpu_state.sprs[SPR_DWCR1] & SPR_DWCR_COUNT) ==
               (cpu_state.sprs[SPR_DWCR1] & SPR_DWCR_MATCH);
      chain2 = watchpoints & (1 << 8);
      break;
    case 10:
      /* TODO: External watchpoint - not yet handled!  */
#if 0
      chain1 = external_watchpoint;
      chain2 = watchpoints & (1 << 9);
#else
      chain1 = chain2 = 0;
#endif
      break;
    default:
      chain1 = DCR_hit;
      chain2 = watchpoints & (bit >> 1);
      break;
    }

    switch((cpu_state.sprs[SPR_DMR1] >> i) & SPR_DMR1_CW0) {
    case 0: match = chain1; break;
    case 1: match = chain1 && chain2; break;
    case 2: match = chain1 || chain2; break;
    }

    /* Increment counters & generate counter break */
    if(match) {
      /* watchpoint did not appear before in this clock cycle */
      if(!(watchpoints & bit)) {
        int counter = (((cpu_state.sprs[SPR_DMR2] & SPR_DMR2_AWTC) >> 2) & bit) ? 1 : 0;
        int enabled = cpu_state.sprs[SPR_DMR2] & (counter ? SPR_DMR2_WCE1 : SPR_DMR2_WCE0);
        if(enabled) {
          uorreg_t count = cpu_state.sprs[SPR_DWCR0 + counter];
          count = (count & ~SPR_DWCR_COUNT) | ((count & SPR_DWCR_COUNT) + 1);
          cpu_state.sprs[SPR_DWCR0 + counter] = count;
        }
        watchpoints |= bit;
      }

      /* should this watchpoint generate a breakpoint? */
      if(((cpu_state.sprs[SPR_DMR2] & SPR_DMR2_WGB) >> 13) & bit)
        breakpoint = 1;
    }
  }

  return breakpoint;
}

static DebugScanChainIDs current_scan_chain = JTAG_CHAIN_GLOBAL;

int DebugGetRegister(oraddr_t address, uorreg_t* data)
{
  int err=0;
  TRACE_(jtag)("Debug get register %x\n",address);
  switch(current_scan_chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
      *data = mfspr(address);
      TRACE_(jtag)("READ  (%"PRIxADDR") = %"PRIxREG"\n", address, *data);
      break;
    case JTAG_CHAIN_TRACE:
      *data = 0;  /* Scan chain not yet implemented */
      break;
    case JTAG_CHAIN_DEVELOPMENT:
      err = get_devint_reg(address,data);
      break;
    case JTAG_CHAIN_WISHBONE:
      err = debug_get_mem(address,data);
      break;
    default:
      err = JTAG_PROXY_INVALID_CHAIN;
    }
  TRACE_(jtag)("!get reg %"PRIxREG"\n", *data);
  return err;
}

int DebugSetRegister(oraddr_t address, uorreg_t data)
{
  int err=0;
  TRACE_(jtag)("Debug set register %"PRIxADDR" <- %"PRIxREG"\n", address, data);
  switch(current_scan_chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
      TRACE_(jtag)("WRITE (%"PRIxADDR") = %"PRIxREG"\n", address, data);
      mtspr(address, data);
      break;
    case JTAG_CHAIN_TRACE:
      err = JTAG_PROXY_ACCESS_EXCEPTION;
      break;
    case JTAG_CHAIN_DEVELOPMENT:
      err = set_devint_reg (address, data);
      break;
    case JTAG_CHAIN_WISHBONE:
      err = debug_set_mem (address, data);
      break;
    default:
      err = JTAG_PROXY_INVALID_CHAIN;
    }
  TRACE_(jtag)("!set reg\n");
  return err;
}

int DebugSetChain(int chain)
{
  TRACE_(jtag)("Debug set chain %x\n",chain);
  switch(chain)
    {
    case JTAG_CHAIN_DEBUG_UNIT:
    case JTAG_CHAIN_TRACE:
    case JTAG_CHAIN_DEVELOPMENT:
    case JTAG_CHAIN_WISHBONE:
      current_scan_chain = chain;
      break;     
    default: /* All other chains not implemented */
      return JTAG_PROXY_INVALID_CHAIN;
    }

  return 0;
}

void sim_reset ();

/* Sets development interface register */
int set_devint_reg(unsigned int address, uint32_t data)
{
  int err = 0;
  unsigned long value = data;
  int old_value;

  switch(address) {
    case DEVELOPINT_MODER: development.moder = value; break;
    case DEVELOPINT_TSEL:  development.tsel = value;  break;
    case DEVELOPINT_QSEL:  development.qsel = value;  break;
    case DEVELOPINT_SSEL:  development.ssel = value;  break;
    case DEVELOPINT_RISCOP:
      old_value = (development.riscop & RISCOP_RESET) != 0;
      development.riscop = value;
      in_reset = (development.riscop & RISCOP_RESET) != 0;
      /* Reset the cpu on the negative edge of RESET */
      if(old_value && !in_reset)
        sim_reset(); /* Reset all units */
      set_stall_state((development.riscop & RISCOP_STALL) != 0);
      break;
    case DEVELOPINT_RECWP0:
    case DEVELOPINT_RECWP1:
    case DEVELOPINT_RECWP2:
    case DEVELOPINT_RECWP3:
    case DEVELOPINT_RECWP4:
    case DEVELOPINT_RECWP5:
    case DEVELOPINT_RECWP6:
    case DEVELOPINT_RECWP7:
    case DEVELOPINT_RECWP8:
    case DEVELOPINT_RECWP9:
    case DEVELOPINT_RECWP10: development.recwp[address - DEVELOPINT_RECWP0] = value; break;
    case DEVELOPINT_RECBP0:  development.recbp = value; break;
    default:
      err = JTAG_PROXY_INVALID_ADDRESS;
      break;
    }
  TRACE_(jtag)("set_devint_reg %08x = %"PRIx32"\n", address, data);
  return err;
}

/* Gets development interface register */
int get_devint_reg(unsigned int address, uint32_t *data)
{
  int err = 0;
  unsigned long value = 0;

  switch(address) {
    case DEVELOPINT_MODER:    value = development.moder; break;
    case DEVELOPINT_TSEL:     value = development.tsel; break;
    case DEVELOPINT_QSEL:     value = development.qsel; break;
    case DEVELOPINT_SSEL:     value = development.ssel; break;
    case DEVELOPINT_RISCOP:   value = development.riscop; break;
    case DEVELOPINT_RECWP0:
    case DEVELOPINT_RECWP1:
    case DEVELOPINT_RECWP2:
    case DEVELOPINT_RECWP3:
    case DEVELOPINT_RECWP4:
    case DEVELOPINT_RECWP5:
    case DEVELOPINT_RECWP6:
    case DEVELOPINT_RECWP7:
    case DEVELOPINT_RECWP8:
    case DEVELOPINT_RECWP9:
    case DEVELOPINT_RECWP10:  value = development.recwp[address - DEVELOPINT_RECWP0]; break;
    case DEVELOPINT_RECBP0:   value = development.recbp; break;
    default:                  err = JTAG_PROXY_INVALID_ADDRESS; break;
  }

  TRACE_(jtag)("get_devint_reg %08x = %08lx\n", address, value);
  *data = value;
  return err;
}

/* Writes to bus address */
int debug_set_mem (oraddr_t address, uint32_t data)
{
  int err = 0;
  TRACE_(jtag)("MEMWRITE (%"PRIxADDR") = %"PRIx32"\n", address, data);


  if(!verify_memoryarea(address))
    err = JTAG_PROXY_INVALID_ADDRESS;
  else {
	  // circumvent the read-only check usually done for mem accesses
	  // data is in host order, because that's what set_direct32 needs
	  set_program32(address, data);
  }
  return err;
}

/* Reads from bus address */
int debug_get_mem(oraddr_t address, uorreg_t *data)
{
  int err = 0;
  if(!verify_memoryarea(address))
    err = JTAG_PROXY_INVALID_ADDRESS;
  else
  {
	  *data=eval_direct32(address, 0, 0);
  }
  TRACE_(jtag)("MEMREAD  (%"PRIxADDR") = %"PRIxADDR"\n", address, *data);
  return err;
}

/* debug_ignore_exception returns 1 if the exception should be ignored. */
int debug_ignore_exception (unsigned long except)
{
  int result = 0;
  unsigned long dsr = cpu_state.sprs[SPR_DSR];
  unsigned long drr = cpu_state.sprs[SPR_DRR];
  
  switch(except) {
    case EXCEPT_RESET:     drr |= result = dsr & SPR_DSR_RSTE; break;
    case EXCEPT_BUSERR:    drr |= result = dsr & SPR_DSR_BUSEE; break;
    case EXCEPT_DPF:       drr |= result = dsr & SPR_DSR_DPFE; break;
    case EXCEPT_IPF:       drr |= result = dsr & SPR_DSR_IPFE; break;
    case EXCEPT_TICK:      drr |= result = dsr & SPR_DSR_TTE; break;
    case EXCEPT_ALIGN:     drr |= result = dsr & SPR_DSR_AE; break;
    case EXCEPT_ILLEGAL:   drr |= result = dsr & SPR_DSR_IIE; break;
    case EXCEPT_INT:       drr |= result = dsr & SPR_DSR_IE; break;
    case EXCEPT_DTLBMISS:  drr |= result = dsr & SPR_DSR_DME; break;
    case EXCEPT_ITLBMISS:  drr |= result = dsr & SPR_DSR_IME; break;
    case EXCEPT_RANGE:     drr |= result = dsr & SPR_DSR_RE; break;
    case EXCEPT_SYSCALL:   drr |= result = dsr & SPR_DSR_SCE; break;
    case EXCEPT_TRAP:      drr |= result = dsr & SPR_DSR_TE; break;
    default:
      break;
  }

  cpu_state.sprs[SPR_DRR] = drr;
  set_stall_state (result != 0);
  return (result != 0);
}

/*--------------------------------------------------[ Debug configuration ]---*/
void debug_enabled(union param_val val, void *dat)
{
  config.debug.enabled = val.int_val;
}

void debug_gdb_enabled(union param_val val, void *dat)
{
  config.debug.gdb_enabled = val.int_val;
}

void debug_server_port(union param_val val, void *dat)
{
  config.debug.server_port = val.int_val;
}

void debug_vapi_id(union param_val val, void *dat)
{
  config.debug.vapi_id = val.int_val;
}

void reg_debug_sec(void)
{
  struct config_section *sec = reg_config_sec("debug", NULL, NULL);

  reg_config_param(sec, "enabled", paramt_int, debug_enabled);
  reg_config_param(sec, "gdb_enabled", paramt_int, debug_gdb_enabled);
  reg_config_param(sec, "server_port", paramt_int, debug_server_port);
  reg_config_param(sec, "vapi_id", paramt_int, debug_vapi_id);
}
