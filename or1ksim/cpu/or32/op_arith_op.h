/* op_arith_op.h -- Micro operations template for arithmetic operations
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


__or_dynop void glue(glue(op_, OP_NAME), _t0_t0_t0)(void)
{
  OP_CAST(t0) = OP_CAST(t0) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t0_t1)(void)
{
  OP_CAST(t0) = OP_CAST(t0) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t0_t2)(void)
{
  OP_CAST(t0) = OP_CAST(t0) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t1_t0)(void)
{
  OP_CAST(t0) = OP_CAST(t1) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t1_t1)(void)
{
  OP_CAST(t0) = OP_CAST(t1) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t1_t2)(void)
{
  OP_CAST(t0) = OP_CAST(t1) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t2_t0)(void)
{
  OP_CAST(t0) = OP_CAST(t2) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t2_t1)(void)
{
  OP_CAST(t0) = OP_CAST(t2) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t0_t2_t2)(void)
{
  OP_CAST(t0) = OP_CAST(t2) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t0_t0)(void)
{
  OP_CAST(t1) = OP_CAST(t0) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t0_t1)(void)
{
  OP_CAST(t1) = OP_CAST(t0) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t0_t2)(void)
{
  OP_CAST(t1) = OP_CAST(t0) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t1_t0)(void)
{
  OP_CAST(t1) = OP_CAST(t1) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t1_t1)(void)
{
  OP_CAST(t1) = OP_CAST(t1) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t1_t2)(void)
{
  OP_CAST(t1) = OP_CAST(t1) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t2_t0)(void)
{
  OP_CAST(t1) = OP_CAST(t2) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t2_t1)(void)
{
  OP_CAST(t1) = OP_CAST(t2) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t1_t2_t2)(void)
{
  OP_CAST(t1) = OP_CAST(t2) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t0_t0)(void)
{
  OP_CAST(t2) = OP_CAST(t0) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t0_t1)(void)
{
  OP_CAST(t2) = OP_CAST(t0) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t0_t2)(void)
{
  OP_CAST(t2) = OP_CAST(t0) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t1_t0)(void)
{
  OP_CAST(t2) = OP_CAST(t1) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t1_t1)(void)
{
  OP_CAST(t2) = OP_CAST(t1) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t1_t2)(void)
{
  OP_CAST(t2) = OP_CAST(t1) OP OP_CAST(t2) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t2_t0)(void)
{
  OP_CAST(t2) = OP_CAST(t2) OP OP_CAST(t0) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t2_t1)(void)
{
  OP_CAST(t2) = OP_CAST(t2) OP OP_CAST(t1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _t2_t2_t2)(void)
{
  OP_CAST(t2) = OP_CAST(t2) OP OP_CAST(t2) OP_EXTRA;
}

#ifdef OP_HAS_IMM
__or_dynop void glue(glue(op_, OP_NAME), _imm_t0_t0)(void)
{
  OP_CAST(t0) = OP_CAST(t0) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t0_t1)(void)
{
  OP_CAST(t0) = OP_CAST(t1) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t0_t2)(void)
{
  OP_CAST(t0) = OP_CAST(t2) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t1_t0)(void)
{
  OP_CAST(t1) = OP_CAST(t0) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t1_t1)(void)
{
  OP_CAST(t1) = OP_CAST(t1) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t1_t2)(void)
{
  OP_CAST(t1) = OP_CAST(t2) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t2_t0)(void)
{
  OP_CAST(t2) = OP_CAST(t0) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t2_t1)(void)
{
  OP_CAST(t2) = OP_CAST(t1) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}

__or_dynop void glue(glue(op_, OP_NAME), _imm_t2_t2)(void)
{
  OP_CAST(t2) = OP_CAST(t2) OP OP_CAST(OP_PARAM1) OP_EXTRA;
}
#endif /* OP_HAS_IMM */
