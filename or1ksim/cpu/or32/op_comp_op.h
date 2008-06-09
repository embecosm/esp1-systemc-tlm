/* op_comp_op.h -- Micro operations template for comparison operations
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


__or_dynop void glue(glue(op_, COMP_NAME), _t0_t0)(void)
{
  if(COMP_CAST(t0) COMP COMP_CAST(t0))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t0_t1)(void)
{
  if(COMP_CAST(t0) COMP COMP_CAST(t1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t0_t2)(void)
{
  if(COMP_CAST(t0) COMP COMP_CAST(t2))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t1_t0)(void)
{
  if(COMP_CAST(t1) COMP COMP_CAST(t0))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t1_t1)(void)
{
  if(COMP_CAST(t1) COMP COMP_CAST(t1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t1_t2)(void)
{
  if(COMP_CAST(t1) COMP COMP_CAST(t2))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t2_t0)(void)
{
  if(COMP_CAST(t2) COMP COMP_CAST(t0))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t2_t1)(void)
{
  if(COMP_CAST(t2) COMP COMP_CAST(t1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _t2_t2)(void)
{
  if(COMP_CAST(t2) COMP COMP_CAST(t2))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
}

__or_dynop void glue(glue(op_, COMP_NAME), _imm_t0)(void)
{
  if(COMP_CAST(t0) COMP COMP_CAST(OP_PARAM1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _imm_t1)(void)
{
  if(COMP_CAST(t1) COMP COMP_CAST(OP_PARAM1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _imm_t2)(void)
{
  if(COMP_CAST(t2) COMP COMP_CAST(OP_PARAM1))
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _null_t0)(void)
{
  if(COMP_CAST(t0) COMP 0)
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _null_t1)(void)
{
  if(COMP_CAST(t1) COMP 0)
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}

__or_dynop void glue(glue(op_, COMP_NAME), _null_t2)(void)
{
  if(COMP_CAST(t2) COMP 0)
    env->sprs[SPR_SR] |= SPR_SR_F;
  else
    env->sprs[SPR_SR] &= ~SPR_SR_F;
  FORCE_RET;
}
