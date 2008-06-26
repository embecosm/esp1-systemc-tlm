/* icache_model.c -- instruction cache simulation
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

/* Cache functions. 
   At the moment this functions only simulate functionality of instruction
   caches and do not influence on fetche/decode/execute stages and timings.
   They are here only to verify performance of various cache configurations.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "icache_model.h"
#include "except.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "stats.h"
#include "sim-config.h"
#include "sprs.h"
#include "sim-config.h"
#include "misc.h"

extern struct dev_memarea *cur_area;
struct ic_set {
  struct {
    uint32_t line[MAX_IC_BLOCK_SIZE];
    oraddr_t tagaddr;  /* tag address */
    int lru;    /* least recently used */
  } way[MAX_IC_WAYS];
} ic[MAX_IC_SETS];

void ic_info()
{
  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP)) {
    PRINTF("ICache not implemented. Set UPR[ICP].\n");
    return;
  }

  PRINTF("Instruction cache %dKB: ", config.ic.nsets * config.ic.blocksize * config.ic.nways / 1024);
  PRINTF("%d ways, %d sets, block size %d bytes\n", config.ic.nways, config.ic.nsets, config.ic.blocksize);
}
                
/* First check if instruction is already in the cache and if it is:
    - increment IC read hit stats,
    - set 'lru' at this way to config.ic.ustates - 1 and
      decrement 'lru' of other ways unless they have reached 0,
    - read insn from the cache line
   and if not:
    - increment IC read miss stats
    - find lru way and entry and replace old tag with tag of the 'fetchaddr'
    - set 'lru' with config.ic.ustates - 1 and decrement 'lru' of other
      ways unless they have reached 0
    - refill cache line
*/

uint32_t ic_simulate_fetch(oraddr_t fetchaddr, oraddr_t virt_addr)
{
  int set, way = -1;
  int i;
  oraddr_t tagaddr;
  uint32_t tmp;

  /* ICache simulation enabled/disabled. */
  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP) ||
      !(cpu_state.sprs[SPR_SR] & SPR_SR_ICE) || insn_ci) {
    tmp = evalsim_mem32(fetchaddr, virt_addr);
    if (cur_area && cur_area->log)
      fprintf (cur_area->log, "[%"PRIxADDR"] -> read %08"PRIx32"\n", fetchaddr,
               tmp);
    return tmp;
  }
  
  /* Which set to check out? */
  set = (fetchaddr / config.ic.blocksize) % config.ic.nsets;
  tagaddr = (fetchaddr / config.ic.blocksize) / config.ic.nsets;
  
  /* Scan all ways and try to find a matching way. */
  for (i = 0; i < config.ic.nways; i++)
    if (ic[set].way[i].tagaddr == tagaddr)
      way = i;
      
  /* Did we find our cached instruction? */
  if (way >= 0) { /* Yes, we did. */
    ic_stats.readhit++;
    
    for (i = 0; i < config.ic.nways; i++)
      if (ic[set].way[i].lru > ic[set].way[way].lru)
        ic[set].way[i].lru--;
    ic[set].way[way].lru = config.ic.ustates - 1;
    runtime.sim.mem_cycles += config.ic.hitdelay;
    return (ic[set].way[way].line[(fetchaddr & (config.ic.blocksize - 1)) >> 2]);
  }
  else {  /* No, we didn't. */
    int minlru = config.ic.ustates - 1;
    int minway = 0;
                
    ic_stats.readmiss++;
                
    for (i = 0; i < config.ic.nways; i++) {
      if (ic[set].way[i].lru < minlru) {
        minway = i;
        minlru = ic[set].way[i].lru;
      }
    }
        
    for (i = 0; i < (config.ic.blocksize); i += 4) {
      tmp = ic[set].way[minway].line[((fetchaddr + i) & (config.ic.blocksize - 1)) >> 2] = 
        /* FIXME: What is the virtual address meant to be? (ie. What happens if
         * we read out of memory while refilling a cache line?) */
        evalsim_mem32((fetchaddr & ~(config.ic.blocksize - 1)) + ((fetchaddr + i) & (config.ic.blocksize - 1)), 0);
      if(!cur_area) {
        ic[set].way[minway].tagaddr = -1;
        ic[set].way[minway].lru = 0;
        return 0;
      } else if (cur_area->log)
        fprintf (cur_area->log, "[%"PRIxADDR"] -> read %08"PRIx32"\n",
                 fetchaddr, tmp);
    }

    ic[set].way[minway].tagaddr = tagaddr;
    for (i = 0; i < config.ic.nways; i++)
      if (ic[set].way[i].lru)
        ic[set].way[i].lru--;
    ic[set].way[minway].lru = config.ic.ustates - 1;
    runtime.sim.mem_cycles += config.ic.missdelay;
    return (ic[set].way[minway].line[(fetchaddr & (config.ic.blocksize - 1)) >> 2]);
  }
}

/* First check if data is already in the cache and if it is:
    - invalidate block if way isn't locked
   otherwise don't do anything.
*/

void ic_inv(oraddr_t dataaddr)
{
  int set, way = -1;
  int i;
  oraddr_t tagaddr;

  if (!(cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP))
    return;

  /* Which set to check out? */
  set = (dataaddr / config.ic.blocksize) % config.ic.nsets;
  tagaddr = (dataaddr / config.ic.blocksize) / config.ic.nsets;

  if (!(cpu_state.sprs[SPR_SR] & SPR_SR_ICE)) {
    for (i = 0; i < config.ic.nways; i++) {
      ic[set].way[i].tagaddr = -1;
      ic[set].way[i].lru = 0;
    }
    return;
  }
  
  /* Scan all ways and try to find a matching way. */
  for (i = 0; i < config.ic.nways; i++)
    if (ic[set].way[i].tagaddr == tagaddr)
      way = i;
      
  /* Did we find our cached data? */
  if (way >= 0) { /* Yes, we did. */
    ic[set].way[way].tagaddr = -1;
    ic[set].way[way].lru = 0;
  }
}

/*-----------------------------------------------------[ IC configuration ]---*/
void ic_enabled(union param_val val, void *dat)
{
  config.ic.enabled = val.int_val;
  if(val.int_val)
    cpu_state.sprs[SPR_UPR] |= SPR_UPR_ICP;
  else {
    cpu_state.sprs[SPR_UPR] &= ~SPR_UPR_ICP;
    config.cpu.upr          &= ~SPR_UPR_ICP;	/* JBP patch */
  }
}

void ic_nsets(union param_val val, void *dat)
{
  if (is_power2(val.int_val) && val.int_val <= MAX_IC_SETS){
    config.ic.nsets = val.int_val;
    cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCS;
    cpu_state.sprs[SPR_ICCFGR] |= log2_int(val.int_val) << 3;
  }
  else {
    char tmp[200];
    sprintf (tmp, "value of power of two and lower or equal than %i expected.", MAX_IC_SETS);
    CONFIG_ERROR(tmp);
  }
}

void ic_nways(union param_val val, void *dat)
{
  if (is_power2(val.int_val) && val.int_val <= MAX_IC_WAYS) {
    config.ic.nways = val.int_val;
    cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_NCW;
    cpu_state.sprs[SPR_ICCFGR] |= log2_int(val.int_val);
  }
  else {
    char tmp[200];
    sprintf (tmp, "value of power of two and lower or equal than %i expected.",
    MAX_IC_WAYS);
    CONFIG_ERROR(tmp);
  }
}

void ic_blocksize(union param_val val, void *dat)
{
  if (is_power2(val.int_val)){
    config.ic.blocksize = val.int_val;
    cpu_state.sprs[SPR_ICCFGR] &= ~SPR_ICCFGR_CBS;
    cpu_state.sprs[SPR_ICCFGR] |= log2_int(val.int_val) << 7;
  } else
    CONFIG_ERROR("value of power of two expected.");
}

void ic_ustates(union param_val val, void *dat)
{
  if (val.int_val >= 2 && val.int_val <= 4)
    config.ic.ustates = val.int_val;
  else
    CONFIG_ERROR("invalid USTATE.");
}

void ic_missdelay(union param_val val, void *dat)
{
  config.ic.missdelay = val.int_val;
}

void ic_hitdelay(union param_val val, void *dat)
{
  config.ic.hitdelay = val.int_val;
}

void reg_ic_sec(void)
{
  struct config_section *sec = reg_config_sec("ic", NULL, NULL);

  reg_config_param(sec, "enabled", paramt_int, ic_enabled);
  reg_config_param(sec, "nsets", paramt_int, ic_nsets);
  reg_config_param(sec, "nways", paramt_int, ic_nways);
  reg_config_param(sec, "blocksize", paramt_int, ic_blocksize);
  reg_config_param(sec, "ustates", paramt_int, ic_ustates);
  reg_config_param(sec, "missdelay", paramt_int, ic_missdelay);
  reg_config_param(sec, "hitdelay", paramt_int, ic_hitdelay);
}
