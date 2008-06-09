/* op_extend_op.h -- Micro operations template for sign extention operations
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


void glue(glue(op_, EXT_NAME), _t0_t0)(void)
{
  register EXT_TYPE x;
  x = t0;
  t0 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t0_t1)(void)
{
  register EXT_TYPE x;
  x = t1;
  t0 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t0_t2)(void)
{
  register EXT_TYPE x;
  x = t2;
  t0 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t1_t0)(void)
{
  register EXT_TYPE x;
  x = t0;
  t1 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t1_t1)(void)
{
  register EXT_TYPE x;
  x = t1;
  t1 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t1_t2)(void)
{
  register EXT_TYPE x;
  x = t2;
  t1 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t2_t0)(void)
{
  register EXT_TYPE x;
  x = t0;
  t2 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t2_t1)(void)
{
  register EXT_TYPE x;
  x = t1;
  t2 = EXT_CAST x;
}

void glue(glue(op_, EXT_NAME), _t2_t2)(void)
{
  register EXT_TYPE x;
  x = t2;
  t2 = EXT_CAST x;
}

