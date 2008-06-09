/* rec_i386.h -- i386 specific parts of the recompile engine
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


#include "common_i386.h"

/* Initialises the recompiler (architechture specific). */
static inline void init_dyn_rec(void)
{
  uint64_t add = UINT32_C(-1) | UINT64_C(4) << 32;

  /* Initialises the scheduler handling for future use.  Because the x86 has a
   * very low number of registers (8), I have to use MMX registers.  I could do
   * load/store combination but that takes up space and is slow.  I use packed
   * doublewords to strut my stuff.  The high 32-bits of the first MMX register
   * is the PC, the low 32-bits is the number of cycles that are still
   * outstanding for the next scheduled job to run.  The second MMX register
   * holds the amount that needs to be added to the two values on each cycle.
   * The third MMX register only holds the value that we must use to update the
   * low 32-bits with */
   asm volatile ("movq %0, %%mm1\n"
                 : : "m" (add));
}

/* Gets the current stack pointer */
static inline void *get_sp(void)
{
  void *stack;
  asm("movl %%esp, %0" : "=rm" (stack));
  return stack;
}

/* Updates the number of cycles that it takes to load 1 instruction */
static inline void upd_cycles_dec(int32_t amount)
{
  useless_x86.val3232.high32 = 4;
  useless_x86.val3232.low32 = amount;
  asm volatile ("movq %0, %%mm1" : : "m" (useless_x86.val64));
}

/* Adds the number of cycles to the next job that would be added anyway */
static inline void sched_add_cycles(void)
{
  asm("movd %%mm1, %%eax\n"
      "\tmovd %%eax, %%mm2\n"
      "paddd %%mm2, %%mm0"
      : : : "eax");
}

/* Adds an arbitary amount to the cycle counter */
static inline void add_to_cycles(int32_t val)
{
  val = -val;
  asm("movd %0, %%mm2\n"
      "paddd %%mm2, %%mm0\n"
      : : "rm" (val));
}
