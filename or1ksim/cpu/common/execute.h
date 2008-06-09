/* execute.h -- Header file for architecture dependent execute.c
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

#if DYNAMIC_EXECUTION
#include "dyn_rec.h"
#endif

struct cpu_state {
  /* General purpose registers. */
  uorreg_t reg[MAX_GPRS];

  /* Sprs */
  uorreg_t sprs[MAX_SPRS];

  /* Effective address of instructions that have an effective address.  This is
   * only used to get dump_exe_log correct */
  oraddr_t insn_ea;

  /* Is current instruction in execution in a delay slot? */
  int delay_insn;

  /* Program counter (and translated PC) */
  oraddr_t pc;

  /* Delay instruction effective address register */
  oraddr_t pc_delay;

  /* Decoding of the just executed instruction.  Only used in analysis(). */
  struct iqueue_entry iqueue;

  /* decoding of the instruction that was executed before this one.  Only used
   * in analysis(). */
  struct iqueue_entry icomplet;

#if DYNAMIC_EXECUTION
  /* Current page in execution */
  struct dyn_page *curr_page;

  /* Pointers to recompiled pages */
  struct dyn_page **dyn_pages;

  /* Micro operation queue.  Only used to speed up recompile_page */
  struct op_queue *opqs;

  /* Set if all temporaries are stored */
  int ts_current;

  /* The contents of the temporaries.  Only used in except_handle */
  uorreg_t t0;
  uorreg_t t1;
  uorreg_t t2;
#endif
};

extern struct cpu_state cpu_state;

#define CURINSN(INSN) (strcmp(cur->insn, (INSN)) == 0)

/*extern machword eval_operand(char *srcoperand,int* breakpoint);
extern void set_operand(char *dstoperand, unsigned long value,int* breakpoint);*/
void dumpreg();
inline void dump_exe_log();
inline int cpu_clock ();
void cpu_reset ();
uorreg_t evalsim_reg(unsigned int regno);
void setsim_reg(unsigned int regno, uorreg_t value);
 
extern oraddr_t pcnext;
int depend_operands(struct iqueue_entry *prev, struct iqueue_entry *next);
