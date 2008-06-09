/* op_mftspr_op.h -- Micro operations template for the m{f,t}spr operations
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


#ifndef ONLY_MTSPR
__or_dynop void glue(glue(glue(op_mfspr_, GPR_T_NAME), _), SPR_T_NAME)(void)
{
  /* FIXME: NPC/PPC Handling is br0ke */
  if(env->sprs[SPR_SR] & SPR_SR_SM) {
    upd_sim_cycles();
    GPR_T = mfspr(SPR_T + OP_PARAM1);
  }
}
#endif

__or_dynop void glue(glue(glue(op_mtspr_, SPR_T_NAME), _), GPR_T_NAME)(void)
{
  /* FIXME: NPC handling DOES NOT WORK like this */
  if(env->sprs[SPR_SR] & SPR_SR_SM) {
    upd_sim_cycles();
    /* Yes, an l.mtspr instruction can cause an exception if the immu is touched
     * it might cause an ITLB miss of instruction page fault. */
    save_t_temporary();
    mtspr(SPR_T + OP_PARAM1, GPR_T);
  }
}

