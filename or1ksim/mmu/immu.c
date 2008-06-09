/* immu.c -- Instruction MMU simulation
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

/* IMMU model, perfectly functional. */

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "immu.h"
#include "abstract.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "stats.h"
#include "sprs.h"
#include "except.h"
#include "sim-config.h"
#include "debug.h"
#include "misc.h"

DEFAULT_DEBUG_CHANNEL(immu);

/* Insn MMU */

/* Precalculates some values for use during address translation */
void init_immu(void)
{
  config.immu.pagesize_log2 = log2_int(config.immu.pagesize);
  config.immu.page_offset_mask = config.immu.pagesize - 1;
  config.immu.page_mask = ~config.immu.page_offset_mask;
  config.immu.vpn_mask = ~((config.immu.pagesize * config.immu.nsets) - 1);
  config.immu.set_mask = config.immu.nsets - 1;
  config.immu.lru_reload = (config.immu.set_mask << 6) & SPR_ITLBMR_LRU;
}

inline uorreg_t *immu_find_tlbmr(oraddr_t virtaddr, uorreg_t **itlbmr_lru)
{
  int set;
  int i;
  oraddr_t vpn;
  uorreg_t *itlbmr;

  /* Which set to check out? */
  set = IADDR_PAGE(virtaddr) >> config.immu.pagesize_log2;
  set &= config.immu.set_mask;
  vpn = virtaddr & config.immu.vpn_mask;

  itlbmr = &cpu_state.sprs[SPR_ITLBMR_BASE(0) + set];
  *itlbmr_lru = itlbmr;

  /* Scan all ways and try to find a matching way. */
  /* FIXME: Should this be reversed? */
  for(i = config.immu.nways; i; i--, itlbmr += (128 * 2)) {
    if(((*itlbmr & config.immu.vpn_mask) == vpn) && (*itlbmr & SPR_ITLBMR_V))
      return itlbmr;
  }

  return NULL;
}

oraddr_t immu_translate(oraddr_t virtaddr)
{
  int i;
  uorreg_t *itlbmr;
  uorreg_t *itlbtr;
  uorreg_t *itlbmr_lru;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_IME) ||
      !(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP)) {
    insn_ci = (virtaddr >= 0x80000000);
    return virtaddr;
  }

  itlbmr = immu_find_tlbmr(virtaddr, &itlbmr_lru);

  /* Did we find our tlb entry? */
  if(itlbmr) { /* Yes, we did. */
    immu_stats.fetch_tlbhit++;
    TRACE("ITLB hit (virtaddr=%"PRIxADDR").\n", virtaddr);
    
    itlbtr = itlbmr + 128;
     
    /* Set LRUs */
    for(i = 0; i < config.immu.nways; i++, itlbmr_lru += (128 * 2)) {
      if(*itlbmr_lru & SPR_ITLBMR_LRU)
        *itlbmr_lru = (*itlbmr_lru & ~SPR_ITLBMR_LRU) |
                                        ((*itlbmr_lru & SPR_ITLBMR_LRU) - 0x40);
    }

    /* This is not necessary `*itlbmr &= ~SPR_ITLBMR_LRU;' since SPR_DTLBMR_LRU
     * is always decremented and the number of sets is always a power of two and
     * as such lru_reload has all bits set that get touched during decrementing
     * SPR_DTLBMR_LRU */
    *itlbmr |= config.immu.lru_reload;

    /* Check if page is cache inhibited */
    insn_ci = *itlbtr & SPR_ITLBTR_CI;

    runtime.sim.mem_cycles += config.immu.hitdelay;

    /* Test for page fault */
    if (cpu_state.sprs[SPR_SR] & SPR_SR_SM) {
      if (!(*itlbtr & SPR_ITLBTR_SXE))
        except_handle(EXCEPT_IPF, virtaddr);
    } else {
      if (!(*itlbtr & SPR_ITLBTR_UXE))
        except_handle(EXCEPT_IPF, virtaddr);
    }

    TRACE("Returning physical address %"PRIxADDR"\n",
          (*itlbtr & SPR_ITLBTR_PPN) | (virtaddr &
                                               (config.immu.page_offset_mask)));
    return (*itlbtr & SPR_ITLBTR_PPN) | (virtaddr &
                                                (config.immu.page_offset_mask));
  }

  /* No, we didn't. */
  immu_stats.fetch_tlbmiss++;
#if 0
  for (i = 0; i < config.immu.nways; i++)
    if (((cpu_state.sprs[SPR_ITLBMR_BASE(i) + set] & SPR_ITLBMR_LRU) >> 6) < minlru)
      minway = i;
      
  cpu_state.sprs[SPR_ITLBMR_BASE(minway) + set] &= ~SPR_ITLBMR_VPN;
  cpu_state.sprs[SPR_ITLBMR_BASE(minway) + set] |= vpn << 12;
  for (i = 0; i < config.immu.nways; i++) {
    uorreg_t lru = cpu_state.sprs[SPR_ITLBMR_BASE(i) + set];
    if (lru & SPR_ITLBMR_LRU) {
      lru = (lru & ~SPR_ITLBMR_LRU) | ((lru & SPR_ITLBMR_LRU) - 0x40);
      cpu_state.sprs[SPR_ITLBMR_BASE(i) + set] = lru;
    }
  }
  cpu_state.sprs[SPR_ITLBMR_BASE(way) + set] &= ~SPR_ITLBMR_LRU;
  cpu_state.sprs[SPR_ITLBMR_BASE(way) + set] |= (config.immu.nsets - 1) << 6;

  /* 1 to 1 mapping */
  cpu_state.sprs[SPR_ITLBTR_BASE(minway) + set] &= ~SPR_ITLBTR_PPN;
  cpu_state.sprs[SPR_ITLBTR_BASE(minway) + set] |= vpn << 12;

  cpu_state.sprs[SPR_ITLBMR_BASE(minway) + set] |= SPR_ITLBMR_V;
#endif

  /* if tlb refill implemented in HW */
  /* return ((cpu_state.sprs[SPR_ITLBTR_BASE(minway) + set] & SPR_ITLBTR_PPN) >> 12) * config.immu.pagesize + (virtaddr % config.immu.pagesize); */
  runtime.sim.mem_cycles += config.immu.missdelay;

  except_handle(EXCEPT_ITLBMISS, virtaddr);
  return 0;
}

/* DESC: try to find EA -> PA transaltion without changing 
 *       any of precessor states. if this is not passible gives up 
 *       (without triggering exceptions).
 *
 * PRMS: virtaddr  - EA for which to find translation 
 *
 * RTRN: 0         - no IMMU, IMMU disabled or ITLB miss
 *       else      - appropriate PA (note it IMMU is not present 
 *                   PA === EA)
 */
oraddr_t peek_into_itlb(oraddr_t virtaddr)
{
  uorreg_t *itlbmr;
  uorreg_t *itlbtr;
  uorreg_t *itlbmr_lru;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_IME) ||
      !(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP)) {
     return(virtaddr);
  }

  itlbmr = immu_find_tlbmr(virtaddr, &itlbmr_lru);

  /* Did we find our tlb entry? */
  if(itlbmr) { /* Yes, we did. */
    itlbtr = itlbmr + 128;

    /* Test for page fault */
    if (cpu_state.sprs[SPR_SR] & SPR_SR_SM) {
      if (!(*itlbtr & SPR_ITLBTR_SXE)) {
	/* no luck, giving up */
	return(0);
      }
    } else {
      if (!(*itlbtr & SPR_ITLBTR_UXE)) {
	/* no luck, giving up */
	return(0);
      }
    }

    return (*itlbtr & SPR_ITLBTR_PPN) | (virtaddr &
                                                (config.immu.page_offset_mask));
  }

  return(0);
}


void itlb_info(void)
{
  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP)) {
    PRINTF("IMMU not implemented. Set UPR[IMP].\n");
    return;
  }

  PRINTF("Insn MMU %dKB: ", config.immu.nsets * config.immu.entrysize * config.immu.nways / 1024);
  PRINTF("%d ways, %d sets, entry size %d bytes\n", config.immu.nways, config.immu.nsets, config.immu.entrysize);
}

/* First check if virtual address is covered by ITLB and if it is:
    - increment ITLB read hit stats,
    - set 'lru' at this way to config.immu.ustates - 1 and
      decrement 'lru' of other ways unless they have reached 0,
    - check page access attributes and invoke IMMU page fault exception
      handler if necessary
   and if not:
    - increment ITLB read miss stats
    - find lru way and entry and invoke ITLB miss exception handler
    - set 'lru' with config.immu.ustates - 1 and decrement 'lru' of other
      ways unless they have reached 0
*/

void itlb_status(int start_set)
{
  int set;
  int way;
  int end_set = config.immu.nsets;

  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_IMP)) {
    PRINTF("IMMU not implemented. Set UPR[IMP].\n");
    return;
  }

  if ((start_set >= 0) && (start_set < end_set))
    end_set = start_set + 1;
  else
    start_set = 0;

  if (start_set < end_set) PRINTF("\nIMMU: ");
  /* Scan set(s) and way(s). */
  for (set = start_set; set < end_set; set++) {
    PRINTF("\nSet %x: ", set);
    for (way = 0; way < config.immu.nways; way++) {
      PRINTF("  way %d: ", way);
      PRINTF("%s\n", dump_spr(SPR_ITLBMR_BASE(way) + set,
                              cpu_state.sprs[SPR_ITLBMR_BASE(way) + set]));
      PRINTF("%s\n", dump_spr(SPR_ITLBTR_BASE(way) + set,
                              cpu_state.sprs[SPR_ITLBTR_BASE(way) + set]));
    }
  }
  if (start_set < end_set) PRINTF("\n");
}

/*---------------------------------------------------[ IMMU configuration ]---*/
void immu_enabled(union param_val val, void *dat)
{
  if(val.int_val)
    cpu_state.sprs[SPR_UPR] |= SPR_UPR_IMP;
  else
    cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_IMP;
  config.immu.enabled = val.int_val;
}

void immu_nsets(union param_val val, void *dat)
{
  if (is_power2(val.int_val) && val.int_val <= 256) {
    config.immu.nsets = val.int_val;
    cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTS;
    cpu_state.sprs[SPR_IMMUCFGR] |= log2_int(val.int_val) << 3;
  }
  else
    CONFIG_ERROR("value of power of two and lower or equal than 256 expected.");
}

void immu_nways(union param_val val, void *dat)
{
  if (val.int_val >= 1 && val.int_val <= 4) {
    config.immu.nways = val.int_val;
    cpu_state.sprs[SPR_IMMUCFGR] &= ~SPR_IMMUCFGR_NTW;
    cpu_state.sprs[SPR_IMMUCFGR] |= val.int_val - 1;
  }
  else
    CONFIG_ERROR("value 1, 2, 3 or 4 expected.");
}

void immu_pagesize(union param_val val, void *dat)
{
  if (is_power2(val.int_val))
    config.immu.pagesize = val.int_val;
  else
    CONFIG_ERROR("value of power of two expected.");
}

void immu_entrysize(union param_val val, void *dat)
{
  if (is_power2(val.int_val))
    config.immu.entrysize = val.int_val;
  else
    CONFIG_ERROR("value of power of two expected.");
}

void immu_ustates(union param_val val, void *dat)
{
  if (val.int_val >= 2 && val.int_val <= 4)
    config.immu.ustates = val.int_val;
  else
    CONFIG_ERROR("invalid USTATE.");
}

void immu_missdelay(union param_val val, void *dat)
{
  config.immu.missdelay = val.int_val;
}

void immu_hitdelay(union param_val val, void *dat)
{
  config.immu.hitdelay = val.int_val;
}

void reg_immu_sec(void)
{
  struct config_section *sec = reg_config_sec("immu", NULL, NULL);

  reg_config_param(sec, "enabled", paramt_int, immu_enabled);
  reg_config_param(sec, "nsets", paramt_int, immu_nsets);
  reg_config_param(sec, "nways", paramt_int, immu_nways);
  reg_config_param(sec, "pagesize", paramt_int, immu_pagesize);
  reg_config_param(sec, "entrysize", paramt_int, immu_entrysize);
  reg_config_param(sec, "ustates", paramt_int, immu_ustates);
  reg_config_param(sec, "missdelay", paramt_int, immu_missdelay);
  reg_config_param(sec, "hitdelay", paramt_int, immu_hitdelay);
}
