/* op_t_reg_mov_op.h -- Micro operations template for reg->temporary operations
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


__or_dynop void glue(op_move_t0_gpr, REG)(void)
{
  t0 = env->reg[REG];
}

__or_dynop void glue(op_move_t1_gpr, REG)(void)
{
  t1 = env->reg[REG];
}

__or_dynop void glue(op_move_t2_gpr, REG)(void)
{
  t2 = env->reg[REG];
}

__or_dynop void glue(glue(op_move_gpr, REG), _t0)(void)
{
  env->reg[REG] = t0;
}

__or_dynop void glue(glue(op_move_gpr, REG), _t1)(void)
{
  env->reg[REG] = t1;
}

__or_dynop void glue(glue(op_move_gpr, REG), _t2)(void)
{
  env->reg[REG] = t2;
}

__or_dynop void glue(glue(op_move_gpr, REG), _pc_delay)(void)
{
  env->pc_delay = env->reg[REG];
  env->delay_insn = 1;
}

