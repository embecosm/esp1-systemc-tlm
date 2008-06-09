/* op_i386.h -- i386 specific support routines for micro operations
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

#define OP_JUMP(x) asm("jmp *%0" : : "rm" (x))

#define FORCE_RET asm volatile ("")

/* Handles the scheduler and PC updateing.  Yes, useing MMX is a requirement. It
 * just won't change.  This must be as compact as possible */
#define HANDLE_SCHED(func, jmp) asm("paddd %%mm1, %%mm0\n" \
                                    "\tmovd %%mm0, %%eax\n" \
                                    "\ttestl %%eax, %%eax\n" \
                                    "\tjg ." jmp "\n" \
                                    "\tcall "#func"\n" \
                                    "\t." jmp ":" : : )

static inline int32_t do_cycles(void)
{
  register uint32_t cycles;

  asm("paddd %%mm1, %%mm0\n"
      "\tmovd %%mm0, %0\n"
      : "=r" (cycles));
  return cycles;
}

/* Joins runtime.sim.mem_cycles with the cycle counter */
static inline void join_mem_cycles(void)
{
  runtime.sim.mem_cycles = -runtime.sim.mem_cycles;
  asm volatile ("movd %0, %%mm2\n"
                "\tpaddd %%mm2, %%mm0"
                : : "m" (runtime.sim.mem_cycles));
  runtime.sim.mem_cycles = 0;
}

static inline void or_longjmp(void *loc) __attribute__((noreturn));
static inline void or_longjmp(void *loc)
{
  /* We push a trampoline address (dyn_ret_stack_prot) onto the stack to be able
   * to detect if any ret instructions found their way into an operation. */
  asm("\tmovl %0, %%eax\n"
      "\tmovl %1, %%esp\n"
      "\tmovl $%2, %%ebp\n"
      "\tpush $dyn_ret_stack_prot\n"
      "\tpush $dyn_ret_stack_prot\n"
      "\tpush $dyn_ret_stack_prot\n"
      "\tjmp *%%eax\n"
      : 
      : "m" (loc),
        "m" (rec_stack_base),
        "m" (cpu_state));
}



