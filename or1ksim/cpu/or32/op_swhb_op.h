/* op_swhb_op.h -- Micro operations template for store operations
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


/* FIXME: Do something with breakpoint */

__or_dynop void glue(glue(op_, S_OP_NAME), _t0_t0)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t0 + OP_PARAM1, t0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t0_t1)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t0 + OP_PARAM1, t1, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t0_t2)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t0 + OP_PARAM1, t2, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t1_t1)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t1 + OP_PARAM1, t1, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t1_t0)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t1 + OP_PARAM1, t0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t1_t2)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t1 + OP_PARAM1, t2, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t2_t0)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t2 + OP_PARAM1, t0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t2_t1)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t2 + OP_PARAM1, t1, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _t2_t2)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t2 + OP_PARAM1, t2, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _imm_t0)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(OP_PARAM1, t0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _imm_t1)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(OP_PARAM1, t1, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _imm_t2)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(OP_PARAM1, t2, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _clear_t0)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t0 + OP_PARAM1, 0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _clear_t1)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t1 + OP_PARAM1, 0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _clear_t2)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(t2 + OP_PARAM1, 0, &breakpoint);
}

__or_dynop void glue(glue(op_, S_OP_NAME), _clear_imm)(void)
{
  int breakpoint;
  upd_sim_cycles();
  save_t_temporary();
  S_FUNC(OP_PARAM1, 0, &breakpoint);
}

