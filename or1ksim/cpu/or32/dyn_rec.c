/* dyn_rec.c -- Dynamic recompiler implementation for or32
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <execinfo.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "immu.h"
#include "abstract.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "except.h"
#include "spr_defs.h"
#include "sim-config.h"
#include "sched.h"

#include "rec_i386.h"
#include "i386_regs.h"

#include "dyn_rec.h"
#include "gen_ops.h"

#include "op_support.h"

/* NOTE: All openrisc (or) addresses in this file are *PHYSICAL* addresses */

/* FIXME: Optimise sorted list adding */

typedef void (*generic_gen_op)(struct op_queue *opq, int end);
typedef void (*imm_gen_op)(struct op_queue *opq, int end, uorreg_t imm);

void gen_l_invalid(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot);

static const generic_gen_op gen_op_move_gpr_t[NUM_T_REGS][32] = {
 { NULL,
   gen_op_move_gpr1_t0,
   gen_op_move_gpr2_t0,
   gen_op_move_gpr3_t0,
   gen_op_move_gpr4_t0,
   gen_op_move_gpr5_t0,
   gen_op_move_gpr6_t0,
   gen_op_move_gpr7_t0,
   gen_op_move_gpr8_t0,
   gen_op_move_gpr9_t0,
   gen_op_move_gpr10_t0,
   gen_op_move_gpr11_t0,
   gen_op_move_gpr12_t0,
   gen_op_move_gpr13_t0,
   gen_op_move_gpr14_t0,
   gen_op_move_gpr15_t0,
   gen_op_move_gpr16_t0,
   gen_op_move_gpr17_t0,
   gen_op_move_gpr18_t0,
   gen_op_move_gpr19_t0,
   gen_op_move_gpr20_t0,
   gen_op_move_gpr21_t0,
   gen_op_move_gpr22_t0,
   gen_op_move_gpr23_t0,
   gen_op_move_gpr24_t0,
   gen_op_move_gpr25_t0,
   gen_op_move_gpr26_t0,
   gen_op_move_gpr27_t0,
   gen_op_move_gpr28_t0,
   gen_op_move_gpr29_t0,
   gen_op_move_gpr30_t0,
   gen_op_move_gpr31_t0 },
 { NULL,
   gen_op_move_gpr1_t1,
   gen_op_move_gpr2_t1,
   gen_op_move_gpr3_t1,
   gen_op_move_gpr4_t1,
   gen_op_move_gpr5_t1,
   gen_op_move_gpr6_t1,
   gen_op_move_gpr7_t1,
   gen_op_move_gpr8_t1,
   gen_op_move_gpr9_t1,
   gen_op_move_gpr10_t1,
   gen_op_move_gpr11_t1,
   gen_op_move_gpr12_t1,
   gen_op_move_gpr13_t1,
   gen_op_move_gpr14_t1,
   gen_op_move_gpr15_t1,
   gen_op_move_gpr16_t1,
   gen_op_move_gpr17_t1,
   gen_op_move_gpr18_t1,
   gen_op_move_gpr19_t1,
   gen_op_move_gpr20_t1,
   gen_op_move_gpr21_t1,
   gen_op_move_gpr22_t1,
   gen_op_move_gpr23_t1,
   gen_op_move_gpr24_t1,
   gen_op_move_gpr25_t1,
   gen_op_move_gpr26_t1,
   gen_op_move_gpr27_t1,
   gen_op_move_gpr28_t1,
   gen_op_move_gpr29_t1,
   gen_op_move_gpr30_t1,
   gen_op_move_gpr31_t1 },
 { NULL,
   gen_op_move_gpr1_t2,
   gen_op_move_gpr2_t2,
   gen_op_move_gpr3_t2,
   gen_op_move_gpr4_t2,
   gen_op_move_gpr5_t2,
   gen_op_move_gpr6_t2,
   gen_op_move_gpr7_t2,
   gen_op_move_gpr8_t2,
   gen_op_move_gpr9_t2,
   gen_op_move_gpr10_t2,
   gen_op_move_gpr11_t2,
   gen_op_move_gpr12_t2,
   gen_op_move_gpr13_t2,
   gen_op_move_gpr14_t2,
   gen_op_move_gpr15_t2,
   gen_op_move_gpr16_t2,
   gen_op_move_gpr17_t2,
   gen_op_move_gpr18_t2,
   gen_op_move_gpr19_t2,
   gen_op_move_gpr20_t2,
   gen_op_move_gpr21_t2,
   gen_op_move_gpr22_t2,
   gen_op_move_gpr23_t2,
   gen_op_move_gpr24_t2,
   gen_op_move_gpr25_t2,
   gen_op_move_gpr26_t2,
   gen_op_move_gpr27_t2,
   gen_op_move_gpr28_t2,
   gen_op_move_gpr29_t2,
   gen_op_move_gpr30_t2,
   gen_op_move_gpr31_t2 } };

static const generic_gen_op gen_op_move_t_gpr[NUM_T_REGS][32] = {
 { NULL,
   gen_op_move_t0_gpr1,
   gen_op_move_t0_gpr2,
   gen_op_move_t0_gpr3,
   gen_op_move_t0_gpr4,
   gen_op_move_t0_gpr5,
   gen_op_move_t0_gpr6,
   gen_op_move_t0_gpr7,
   gen_op_move_t0_gpr8,
   gen_op_move_t0_gpr9,
   gen_op_move_t0_gpr10,
   gen_op_move_t0_gpr11,
   gen_op_move_t0_gpr12,
   gen_op_move_t0_gpr13,
   gen_op_move_t0_gpr14,
   gen_op_move_t0_gpr15,
   gen_op_move_t0_gpr16,
   gen_op_move_t0_gpr17,
   gen_op_move_t0_gpr18,
   gen_op_move_t0_gpr19,
   gen_op_move_t0_gpr20,
   gen_op_move_t0_gpr21,
   gen_op_move_t0_gpr22,
   gen_op_move_t0_gpr23,
   gen_op_move_t0_gpr24,
   gen_op_move_t0_gpr25,
   gen_op_move_t0_gpr26,
   gen_op_move_t0_gpr27,
   gen_op_move_t0_gpr28,
   gen_op_move_t0_gpr29,
   gen_op_move_t0_gpr30,
   gen_op_move_t0_gpr31 },
 { NULL,
   gen_op_move_t1_gpr1,
   gen_op_move_t1_gpr2,
   gen_op_move_t1_gpr3,
   gen_op_move_t1_gpr4,
   gen_op_move_t1_gpr5,
   gen_op_move_t1_gpr6,
   gen_op_move_t1_gpr7,
   gen_op_move_t1_gpr8,
   gen_op_move_t1_gpr9,
   gen_op_move_t1_gpr10,
   gen_op_move_t1_gpr11,
   gen_op_move_t1_gpr12,
   gen_op_move_t1_gpr13,
   gen_op_move_t1_gpr14,
   gen_op_move_t1_gpr15,
   gen_op_move_t1_gpr16,
   gen_op_move_t1_gpr17,
   gen_op_move_t1_gpr18,
   gen_op_move_t1_gpr19,
   gen_op_move_t1_gpr20,
   gen_op_move_t1_gpr21,
   gen_op_move_t1_gpr22,
   gen_op_move_t1_gpr23,
   gen_op_move_t1_gpr24,
   gen_op_move_t1_gpr25,
   gen_op_move_t1_gpr26,
   gen_op_move_t1_gpr27,
   gen_op_move_t1_gpr28,
   gen_op_move_t1_gpr29,
   gen_op_move_t1_gpr30,
   gen_op_move_t1_gpr31 },
 { NULL,
   gen_op_move_t2_gpr1,
   gen_op_move_t2_gpr2,
   gen_op_move_t2_gpr3,
   gen_op_move_t2_gpr4,
   gen_op_move_t2_gpr5,
   gen_op_move_t2_gpr6,
   gen_op_move_t2_gpr7,
   gen_op_move_t2_gpr8,
   gen_op_move_t2_gpr9,
   gen_op_move_t2_gpr10,
   gen_op_move_t2_gpr11,
   gen_op_move_t2_gpr12,
   gen_op_move_t2_gpr13,
   gen_op_move_t2_gpr14,
   gen_op_move_t2_gpr15,
   gen_op_move_t2_gpr16,
   gen_op_move_t2_gpr17,
   gen_op_move_t2_gpr18,
   gen_op_move_t2_gpr19,
   gen_op_move_t2_gpr20,
   gen_op_move_t2_gpr21,
   gen_op_move_t2_gpr22,
   gen_op_move_t2_gpr23,
   gen_op_move_t2_gpr24,
   gen_op_move_t2_gpr25,
   gen_op_move_t2_gpr26,
   gen_op_move_t2_gpr27,
   gen_op_move_t2_gpr28,
   gen_op_move_t2_gpr29,
   gen_op_move_t2_gpr30,
   gen_op_move_t2_gpr31 } };

static const imm_gen_op calc_insn_ea_table[NUM_T_REGS] =
 { gen_op_calc_insn_ea_t0, gen_op_calc_insn_ea_t1, gen_op_calc_insn_ea_t2 };

/* Linker stubs.  This will allow the linker to link in op.o.  The relocations
 * that the linker does for these will be irrelevent anyway, since we patch the
 * relocations during recompilation. */
uorreg_t __op_param1;
uorreg_t __op_param2;
uorreg_t __op_param3;

/* The number of bytes that a dynamicly recompiled page should be enlarged by */
#define RECED_PAGE_ENLARGE_BY 51200

/* The number of entries that the micro operations array in op_queue should be
 * enlarged by */
#define OPS_ENLARGE_BY 5

#define T_NONE (-1)

void *rec_stack_base;

/* FIXME: Put this into some header */
extern int do_stats;

static int sigsegv_state = 0;
static void *sigsegv_addr = NULL;

void dyn_ret_stack_prot(void);

void dyn_sigsegv_debug(int u, siginfo_t *siginf, void *dat)
{
  struct dyn_page *dp;
  FILE *f;
  char filen[18]; /* 18 == strlen("or_page.%08x") + 1 */
  void *stack;
  int i, j;
  void *trace[11];
  int num_trace;
  char **trace_names;
  struct sigcontext *sigc = dat;

  if(!sigsegv_state) {
    sigsegv_addr = siginf->si_addr;
  } else {
    fprintf(stderr, "Nested SIGSEGV occured, dumping next chuck of info\n");
    sigsegv_state++;
  }

  /* First dump all the data that does not need dereferenceing to get */
  switch(sigsegv_state) {
  case 0:
    fflush(stderr);
    fprintf(stderr, "Segmentation fault on acces to %p at 0x%08lx, (or address: 0x%"PRIxADDR")\n\n",
            sigsegv_addr, sigc->eip, get_pc());
    sigsegv_state++;
  case 1:
    /* Run through the recompiled pages, dumping them to disk as we go */
    for(i = 0; i < (2 << (32 - config.dmmu.pagesize_log2)); i++) {
      dp = cpu_state.dyn_pages[i];
      if(!dp)
        continue;
      fprintf(stderr, "Dumping%s page 0x%"PRIxADDR" recompiled to %p (len: %u) to disk\n",
             dp->dirty ? " dirty" : "", dp->or_page, dp->host_page,
             dp->host_len);
      fflush(stdout);

      sprintf(filen, "or_page.%"PRIxADDR, dp->or_page);
      if(!(f = fopen(filen, "w"))) {
        fprintf(stderr, "Unable to open %s to dump the recompiled page to: %s\n",
                filen, strerror(errno));
        continue;
      }
      if(fwrite(dp->host_page, dp->host_len, 1, f) < 1)
        fprintf(stderr, "Unable to write recompiled data to file: %s\n",
                strerror(errno));

      fclose(f);
    }
    sigsegv_state++;
  case 2:
    /* Dump the contents of the stack */
    fprintf(stderr, "Stack dump: ");
    fflush(stderr);

    num_trace = backtrace(trace, 10);

    trace[num_trace++] = (void *)sigc->eip;
    trace_names = backtrace_symbols(trace, num_trace);

    stack = get_sp();
    fprintf(stderr, "(of stack at %p, base: %p)\n", stack, rec_stack_base);
    fflush(stderr);
    for(i = 0; stack < rec_stack_base; i++, stack += 4) {
      fprintf(stderr, " <%i> 0x%08x", i, *(uint32_t *)stack);
      /* Try to find a symbolic name with this entry */
      for(j = 0; j < num_trace; j++) {
        if(trace[j] == *(void **)stack)
          fprintf(stderr, " <%s>", trace_names[j]);
      }
      fprintf(stderr, "\n");
      fflush(stderr);
    }
    fprintf(stderr, "Fault at: 0x%08lx <%s>\n", sigc->eip,
            trace_names[num_trace - 1]);
    sigsegv_state++;
  case 3:
    sim_done();
  }
}

struct dyn_page *new_dp(oraddr_t page)
{
  struct dyn_page *dp = malloc(sizeof(struct dyn_page));
  dp->or_page = IADDR_PAGE(page);

  dp->locs = malloc(sizeof(void *) * (config.immu.pagesize / 4));

  dp->host_len = 0;
  dp->host_page = NULL;
  dp->dirty = 1;

  cpu_state.dyn_pages[dp->or_page >> config.immu.pagesize_log2] = dp;
  return dp;
}

/* This is called whenever the immu is either enabled/disabled or reconfigured
 * while enabled.  This checks if an itlb miss would occour and updates the immu
 * hit delay counter */
void recheck_immu(int got_en_dis)
{
  oraddr_t addr = get_pc();
  extern int immu_ex_from_insn;

  if(cpu_state.delay_insn) {
    /* If an instruction pagefault or an ITLB miss would occur, it must appear
     * to have come from the jumped-to address */
    if(IADDR_PAGE(addr) == IADDR_PAGE(cpu_state.pc_delay)) {
      immu_ex_from_insn = 1;
      immu_translate(addr + 4);
      immu_ex_from_insn = 0;
      runtime.sim.mem_cycles = 0;
    }
    return;
  }

  if(IADDR_PAGE(addr) == IADDR_PAGE(addr + 4)) {
    /* If the next instruction is on another page then the immu will be checked
     * when the jump to the next page happens */
    immu_ex_from_insn = 1;
    immu_translate(addr + 4);
    immu_ex_from_insn = 0;
    /* If we had am immu hit then runtime.sim.mem_cycles will hold the value
     * config.immu.hitdelay, but this value is added to the cycle when the next
     * instruction is run */
    runtime.sim.mem_cycles = 0;
  }
  /* Only update the cycle decrementer if the mmu got enabled or disabled */
  if(got_en_dis == IMMU_GOT_ENABLED)
    /* Add the mmu hit delay to the cycle counter */
    upd_cycles_dec(cpu_state.curr_page->delayr - config.immu.hitdelay);
  else if(got_en_dis == IMMU_GOT_DISABLED) {
    upd_cycles_dec(cpu_state.curr_page->delayr);
    /* Since we updated the cycle decrementer above the immu hit delay will not
     * be added to the cycle counter for this instruction.  Compensate for this
     * by adding it now */
    /* FIXME: This is not correct here.  In the complex execution model the hit
     * delay is added to runtime.sim.mem_cycles which is only joined with the
     * cycle counter after analysis() and before the scheduler would run.
     * Therefore the scheduler will still be correct but analysis() will produce
     * wrong results just for this one instruction. */
    add_to_cycles(config.immu.hitdelay);
  }
}

/* Runs the scheduler.  Called from except_handler (and dirtyfy_page below) */
void run_sched_out_of_line(int add_normal)
{
  oraddr_t pc = get_pc();
  extern int immu_ex_from_insn;

  if(!cpu_state.ts_current)
    upd_reg_from_t(pc, 0);

  if(add_normal && do_stats) {
    cpu_state.iqueue.insn_addr = pc;
    cpu_state.iqueue.insn = eval_insn_direct(pc, !immu_ex_from_insn);
    cpu_state.iqueue.insn_index = insn_decode(cpu_state.iqueue.insn);
    runtime.cpu.instructions++;
    analysis(&cpu_state.iqueue);
  }

  /* Run the scheduler */
  if(add_normal)
    sched_add_cycles();

  op_join_mem_cycles();
  upd_sim_cycles();
  if(scheduler.job_queue->time <= 0)
    do_scheduler();
}

/* Signals a page as dirty */
static void dirtyfy_page(struct dyn_page *dp)
{
  oraddr_t check;

  printf("Dirtyfying page 0x%"PRIxADDR"\n", dp->or_page);

  dp->dirty = 1;

  /* If the execution is currently in the page that was touched then recompile
   * it now and jump back to the point of execution */
  check = cpu_state.delay_insn ? cpu_state.pc_delay : get_pc() + 4;
  if(IADDR_PAGE(check) == dp->or_page) {
    run_sched_out_of_line(1);
    recompile_page(dp);

    cpu_state.delay_insn = 0;

    /* Jump out to the next instruction */
    do_jump(check);
  }
}

/* Checks to see if a write happened to a recompiled page.  If so marks it as
 * dirty */
void dyn_checkwrite(oraddr_t addr)
{
  /* FIXME: Do this with mprotect() */
  struct dyn_page *dp = cpu_state.dyn_pages[addr >> config.immu.pagesize_log2];

  /* Since the locations 0x0-0xff are nearly always written to in an exception
   * handler, ignore any writes to these locations.  If code ends up jumping
   * out there, we'll recompile when the jump actually happens. */
  if((addr > 0x100) && dp && !dp->dirty)
    dirtyfy_page(dp);
}

static void ship_gprs_out_t(struct op_queue *opq, int end, unsigned int *reg_t)
{
  int i;

  /* Before takeing the temporaries out, temporarily remove the op_do_sched
   * operation such that dyn_page->ts_bound shall be correct before the
   * scheduler runs */
  if(end && opq->num_ops && (opq->ops[opq->num_ops - 1] == op_do_sched_indx)) {
    opq->num_ops--;
    ship_gprs_out_t(opq, end, reg_t);
    gen_op_do_sched(opq, 1);
    return;
  }

  for(i = 0; i < NUM_T_REGS; i++) {
    if(reg_t[i] < 32)
      gen_op_move_gpr_t[i][reg_t[i]](opq, end);
  }
}

static int find_unused_t(unsigned int *pres_t, unsigned int *reg_t)
{
  int empty = -1; /* Invalid */
  int i;

  /* Try to find a temporary that does not contain a register and is not
   * needed to be preserved */
  for(i = 0; i < NUM_T_REGS; i++) {
    if(!pres_t[i]) {
      empty = i;
      if(reg_t[i] > 31)
        return i;
    }
  }
  return empty;
}

/* Checks if there is enough space in dp->host_page, if not grow it */
void *enough_host_page(struct dyn_page *dp, void *cur, unsigned int *len,
                       unsigned int amount)
{
  unsigned int used = cur - dp->host_page;

  /* The array is long enough */
  if((used + amount) <= *len)
    return cur;

  /* Reallocate */
  *len += RECED_PAGE_ENLARGE_BY;

  if(!(dp->host_page = realloc(dp->host_page, *len))) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }

  return dp->host_page + used;
}

/* Adds an operation to the opq */
void add_to_opq(struct op_queue *opq, int end, int op)
{
  if(opq->num_ops == opq->ops_len) {
    opq->ops_len += OPS_ENLARGE_BY;
    if(!(opq->ops = realloc(opq->ops, opq->ops_len * sizeof(int)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }
  }

  if(end)
    opq->ops[opq->num_ops] = op;
  else {
    /* Shift everything over by one */
    memmove(opq->ops + 1, opq->ops, opq->num_ops* sizeof(int));
    opq->ops[0] = op;
  }

  opq->num_ops++;
}

static void gen_op_mark_loc(struct op_queue *opq, int end)
{
  add_to_opq(opq, end, op_mark_loc_indx);
}

/* Adds a parameter to the opq */
void add_to_op_params(struct op_queue *opq, int end, unsigned long param)
{
  if(opq->num_ops_param == opq->ops_param_len) {
    opq->ops_param_len += OPS_ENLARGE_BY;
    if(!(opq->ops_param = realloc(opq->ops_param, opq->ops_param_len * sizeof(int)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }
  }

  if(end)
    opq->ops_param[opq->num_ops_param] = param;
  else {
    /* Shift everything over by one */
    memmove(opq->ops_param + 1, opq->ops_param, opq->num_ops_param);
    opq->ops_param[0] = param;
  }

  opq->num_ops_param++;
}

/* Function to guard against rogue ret instructions in the operations */
void dyn_ret_stack_prot(void)
{
  fprintf(stderr, "An operation (I have no clue which) has a ret statement in it\n");
  fprintf(stderr, "Good luck debugging it!\n");

  exit(1);
}

/* Jumps out to some Openrisc address */
void jump_dyn_code(oraddr_t addr)
{
  set_pc(addr);
  do_jump(addr);
}

/* Initialises the recompiler */
void init_dyn_recomp(void)
{
  struct sigaction sigact;
  struct op_queue *opq;
  unsigned int i;

  cpu_state.opqs = NULL;

  /* Allocate the operation queue list (+1 for the page chaining) */
  for(i = 0; i < (config.immu.pagesize / 4) + 1; i++) {
    if(!(opq = malloc(sizeof(struct op_queue)))) {
      fprintf(stderr, "OOM\n");
      exit(1);
    }

    /* initialise some fields */
    opq->ops_len = 0;
    opq->ops = NULL;
    opq->ops_param_len = 0;
    opq->ops_param = NULL;
    opq->xref = 0;

    if(cpu_state.opqs)
      cpu_state.opqs->prev = opq;

    opq->next = cpu_state.opqs;
    cpu_state.opqs = opq;
  }

  opq->prev = NULL;

  /* Just some value that we'll use as the base for our stack */
  rec_stack_base = get_sp();

  cpu_state.curr_page = NULL;
  if(!(cpu_state.dyn_pages = malloc(sizeof(void *) * (2 << (32 -
                                                config.immu.pagesize_log2))))) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }
  memset(cpu_state.dyn_pages, 0,
         sizeof(void *) * (2 << (32 - config.immu.pagesize_log2)));

  /* Register our segmentation fault handler */
  sigact.sa_sigaction = dyn_sigsegv_debug;
  memset(&sigact.sa_mask, 0, sizeof(sigact.sa_mask));
  sigact.sa_flags = SA_SIGINFO | SA_NOMASK;
  if(sigaction(SIGSEGV, &sigact, NULL))
    printf("WARN: Unable to install SIGSEGV handler! Don't expect to be able to debug the recompiler.\n");

  /* Do architecture specific initialisation */
  init_dyn_rec();

  /* FIXME: Find a better place for this */
    { /* Needed by execution */
      extern int do_stats;
      do_stats = config.cpu.dependstats || config.cpu.superscalar || config.cpu.dependstats
              || config.sim.history || config.sim.exe_log;
    }

  printf("Recompile engine up and running\n");
}

/* Adds code to the opq for the instruction pointed to by addr */
static void recompile_insn(struct op_queue *opq, oraddr_t addr, int delay_insn)
{
  unsigned int insn_index;
  unsigned int pres_t[NUM_T_REGS]; /* Which temporary to preserve */
  orreg_t param[3];
  int i, j, k;
  int param_t[3]; /* Which temporary the parameters reside in */
  int param_r[3]; /* is parameter a register */
  int param_num;
  uint32_t insn;
  int breakp;
  struct insn_op_struct *opd;

  breakp = 0;
  insn = eval_insn(addr, &breakp);

  /* FIXME: If a breakpoint is set at this location, insert exception code */
  if(breakp) {
    fprintf(stderr, "FIXME: Insert breakpoint code\n");
  }

  insn_index = insn_decode(insn);

  /* Copy over the state of the temporaries to the next opq */
  memcpy(opq->reg_t_d, opq->reg_t, sizeof(opq->reg_t));

  /* Check if we have an illegal instruction */
  if(insn_index == -1) {
    gen_l_invalid(opq, NULL, NULL, delay_insn);
    return;
  }

  /* If we are recompileing an instruction that has a delay slot and is in the
   * delay slot, ignore it.  This is undefined behavour. */
  if(delay_insn && (or32_opcodes[insn_index].flags & OR32_IF_DELAY))
    return;

  /* figure out instruction operands */
  for(i = 0; i < NUM_T_REGS; i++)
    pres_t[i] = 0;

  param_t[0] = T_NONE;
  param_t[1] = T_NONE;
  param_t[2] = T_NONE;
  param_r[0] = 0;
  param_r[1] = 0;
  param_r[2] = 0;
  param_num = 0;

  opd = op_start[insn_index];
  while(1) {
    param[param_num] = eval_operand_val(insn, opd);

    if(opd->type & OPTYPE_REG) {
      /* check which temporary the register is in, if any */
      for(i = 0; i < NUM_T_REGS; i++) {
        if(opq->reg_t_d[i] == param[param_num]) {
          param_t[param_num] = i;
          pres_t[i] = 1;
        }
      }
    }

    param_num++;
    while(!(opd->type & OPTYPE_OP)) opd++;
    if(opd->type & OPTYPE_LAST)
      break;
    opd++;
  }

  /* Jump instructions are special since they have a delay slot and thus they
   * need to control the exact operation sequence.  Special case these here to
   * avoid haveing loads of if(!(.& OR32_IF_DELAY)) below */
  if(or32_opcodes[insn_index].flags & OR32_IF_DELAY) {
    /* Ship the jump-to register out (if it exists).  It requires special
     * handleing, which is done in gen_j_reg. */
    for(i = 0; i < NUM_T_REGS; i++) {
      if(pres_t[i]) {
        gen_op_move_gpr_t[i][opq->reg_t_d[i]](opq->prev, 1);
        opq->reg_t_d[i] = 32;
        opq->reg_t[i] = 32;
      }
    }

    /* FIXME: Do this in a more elegent way */
    if(!strncmp(or32_opcodes[insn_index].name, "l.jal", 5)) {
      /* In the case of a l.jal instruction, make sure that LINK_REGNO is not in
       * a temporary.  The problem is that the l.jal(r) instruction stores the
       * `return address' in LINK_REGNO.  The temporaries are shiped out only
       * after the delay slot instruction has executed and so it overwrittes the
       * `return address'. */
      for(k = 0; k < NUM_T_REGS; k++) {
        if(opq->reg_t_d[k] == LINK_REGNO) {
          gen_op_move_gpr_t[k][LINK_REGNO](opq, 1);
          opq->reg_t_d[k] = 32;
          break;
        }
      }
    }

    /* Jump instructions don't have a disposition */
    or32_opcodes[insn_index].exec(opq, param_t, param, delay_insn);

    /* Analysis is done by the individual jump instructions */
    /* Jump instructions don't touch runtime.sim.mem_cycles */
    /* Jump instructions run their own scheduler */
    return;
  }

  /* Before an exception takes place, all registers must be stored. */
  if((or32_opcodes[insn_index].func_unit == it_exception)) {
    if(opq->prev) {
      ship_gprs_out_t(opq->prev, 1, opq->reg_t_d);
      for(i = 0; i < NUM_T_REGS; i++) {
        opq->reg_t_d[i] = 32;
        opq->reg_t[i] = 32;
      }
    }
  }

  opd = op_start[insn_index];

  for(j = 0; j < param_num; j++, opd++) {
    while(!(opd->type & OPTYPE_OP)) opd++;
    if(!(opd->type & OPTYPE_REG))
      continue;

    /* Never, ever, move r0 into a temporary */
    if(!param[j])
      continue;

    /* Check if this register has been moved into a temporary in a previous
     * operand */
    for(k = 0; k < NUM_T_REGS; k++) {
      if(opq->reg_t_d[k] == param[j]) {
        /* Yes, this register is already in a temporary */
        if(or32_opcodes[insn_index].func_unit != it_jump) {
          pres_t[k] = 1;
          param_t[j] = k;
        }
        break;
      }
    }
    if(k != NUM_T_REGS)
      continue;

    if(param_t[j] != T_NONE)
      continue;

    /* Search for an unused temporary */
    k = find_unused_t(pres_t, opq->reg_t_d);
    if(opq->reg_t_d[k] < 32) {
      /* FIXME: Only ship the temporary out if it has been used as a destination
       * register */
      gen_op_move_gpr_t[k][opq->reg_t_d[k]](opq->prev, 1);
      opq->reg_t[k] = 32;
      opq->reg_t_d[k] = 32;
    }
    pres_t[k] = 1;
    opq->reg_t_d[k] = param[j];
    param_t[j] = k;
    /* FIXME: Only generate code to move the register into a temporary if it
     *        is used as a source operand */
    gen_op_move_t_gpr[k][opq->reg_t_d[k]](opq, 0);
  }

  /* To get the execution log correct for instructions like l.lwz r4,0(r4) the
   * effective address needs to be calculated before the instruction is
   * simulated */
  if(do_stats) {
    /* Find any disposition in the instruction */
    opd = op_start[insn_index];
    for(j = 0; j < param_num; j++, opd++) {
      while(!(opd->type & OPTYPE_OP)) opd++;
      if(!(opd->type & OPTYPE_DIS))
        continue;

      if(!param[j + 1])
        gen_op_store_insn_ea(opq, 1, param[j]);
      else
        calc_insn_ea_table[param_t[j + 1]](opq, 1, param[j]);
    }
  }

  or32_opcodes[insn_index].exec(opq, param_t, param, delay_insn);

  if(or32_opcodes[insn_index].func_unit != it_exception) {
    if(do_stats)
      gen_op_analysis(opq, 1, insn_index, insn);
  }

  /* The call to join_mem_cycles() could be put into the individual operations
   * that emulate the load/store instructions, but then it would be added to
   * the cycle counter before analysis() is called, which is not how the complex
   * execution model does it. */
  if((or32_opcodes[insn_index].func_unit == it_load) ||
     (or32_opcodes[insn_index].func_unit == it_store))
    gen_op_join_mem_cycles(opq, 1);

  /* Delay slot instructions get a special scheduler, thus don't generate it
   * here */
  if((or32_opcodes[insn_index].func_unit != it_exception) && !delay_insn)
    gen_op_do_sched(opq, 1);
}

/* Recompiles the page associated with *dyn */
void recompile_page(struct dyn_page *dyn)
{
  unsigned int j;
  struct op_queue *opq = cpu_state.opqs;
  oraddr_t rec_addr = dyn->or_page;
  oraddr_t rec_page = dyn->or_page;
  void **loc;

  /* The start of the next page */
  rec_page += config.immu.pagesize;

  printf("Recompileing page %"PRIxADDR"\n", rec_addr);
  fflush(stdout);

  /* Mark all temporaries as not containing a register */
  for(j = 0; j < NUM_T_REGS; j++)
    opq->reg_t[j] = 32; /* Out-of-range registers */

  dyn->delayr = -verify_memoryarea(rec_addr)->ops.delayr;

  opq->num_ops = 0;
  opq->num_ops_param = 0;

  /* Insert code to check if the first instruction is exeucted in a delay slot*/
  gen_op_check_delay_slot(opq, 1, 0);
  recompile_insn(opq, rec_addr, 1);
  ship_gprs_out_t(opq, 1, opq->reg_t_d);
  gen_op_do_sched_delay(opq, 1);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_jump_delay(opq, 1);
  gen_op_mark_loc(opq, 1);

  for(j = 0; j < NUM_T_REGS; j++)
    opq->reg_t[j] = 32; /* Out-of-range registers */

  for(; rec_addr < rec_page; rec_addr += 4, opq = opq->next) {
    if(opq->prev) {
      opq->num_ops = 0;
      opq->num_ops_param = 0;
    }
    opq->jump_local = -1;
    opq->not_jump_loc = -1;

    opq->insn_addr = rec_addr;

    /* Check if this location is cross referenced */
    if(opq->xref) {
      /* If the current address is cross-referenced, the temporaries shall be
       * in an undefined state, so we must assume that no registers reside in
       * them */
      /* Ship out the current set of registers from the temporaries */
      if(opq->prev)
        ship_gprs_out_t(opq->prev, 1, opq->reg_t);

      for(j = 0; j < NUM_T_REGS; j++)
        opq->reg_t[j] = 32;
    }

    recompile_insn(opq, rec_addr, 0);

    /* Store the state of the temporaries */
    memcpy(opq->next->reg_t, opq->reg_t_d, sizeof(opq->reg_t));
  }

  dyn->dirty = 0;

  /* Store the state of the temporaries */
  dyn->ts_bound[config.immu.pagesize >> 2] = dyn->ts_during[j];

  /* Ship temporaries out to the corrisponding registers */
  ship_gprs_out_t(opq->prev, 1, opq->reg_t);

  opq->num_ops = 0;
  opq->num_ops_param = 0;
  opq->not_jump_loc = -1;
  opq->jump_local = -1;

  /* Insert code to jump to the next page */
  gen_op_set_ts_current(opq, 1);
  gen_op_do_jump(opq, 1);

  /* Generate the code */
  gen_code(cpu_state.opqs, dyn);

  /* Fix up the locations */
  for(loc = dyn->locs; loc < &dyn->locs[config.immu.pagesize / 4]; loc++)
    *loc += (unsigned int)dyn->host_page;

  cpu_state.opqs->ops_param[0] += (unsigned int)dyn->host_page;

  /* Search for page-local jumps */
  opq = cpu_state.opqs;
  for(j = 0; j < (config.immu.pagesize / 4); opq = opq->next, j++) {
    if(opq->jump_local != -1)
      opq->ops_param[opq->jump_local] =
                              (unsigned int)dyn->locs[opq->jump_local_loc >> 2];

    if(opq->not_jump_loc != -1)
      opq->ops_param[opq->not_jump_loc] = (unsigned int)dyn->locs[j + 1];

    /* Store the state of the temporaries into dyn->ts_bound */
    dyn->ts_bound[j] = 0;
    if(opq->reg_t[0] < 32)
      dyn->ts_bound[j] = opq->reg_t[0];
    if(opq->reg_t[1] < 32)
      dyn->ts_bound[j] |= opq->reg_t[1] << 5;
    if(opq->reg_t[2] < 32)
      dyn->ts_bound[j] |= opq->reg_t[2] << 10;

    dyn->ts_during[j] = 0;
    if(opq->reg_t_d[0] < 32)
      dyn->ts_during[j] = opq->reg_t_d[0];
    if(opq->reg_t_d[1] < 32)
      dyn->ts_during[j] |= opq->reg_t_d[1] << 5;
    if(opq->reg_t_d[2] < 32)
      dyn->ts_during[j] |= opq->reg_t_d[2] << 10;
  }

  /* Patch the relocations */
  patch_relocs(cpu_state.opqs, dyn->host_page);

  /* FIXME: Fix the issue below in a more elegent way */
  /* Since eval_insn is called to get the instruction, runtime.sim.mem_cycles is
   * updated but the recompiler expectes it to start a 0, so reset it */
  runtime.sim.mem_cycles = 0;
}

/* Returns non-zero if the jump is into this page, 0 otherwise */
static int find_jump_loc(oraddr_t j_ea, struct op_queue *opq)
{
  int i;

  /* Mark the jump as non page local if the delay slot instruction is on the
   * next page to the jump instruction.  This should not be needed */
  if((IADDR_PAGE(j_ea) != IADDR_PAGE(opq->insn_addr)) ||
     (IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)))
    /* We can't do anything as the j_ea (as passed to find_jump_loc) is a
     * VIRTUAL offset and the next physical page may not be the next VIRTUAL
     * page */
    return 0;

  /* The jump is into the page currently undergoing dynamic recompilation */

  /* If we haven't got to the location of the jump, everything is ok */
  if(j_ea > opq->insn_addr) {
    /* Find the corissponding opq and mark it as cross referenced */
    for(i = (j_ea - opq->insn_addr) / 4; i; i--)
      opq = opq->next;
    opq->xref = 1;
    return 1;
  }

  /* Insert temporary -> register code before the jump ea and register ->
   * temporary at the x-ref address */
  for(i = (opq->insn_addr - j_ea) / 4; i; i--)
    opq = opq->prev;

  if(!opq->prev)
    /* We're at the begining of a page, no need to do anything */
    return 1;

  /* Found location, insert code */

  ship_gprs_out_t(opq->prev, 1, opq->reg_t);

  for(i = 0; i < NUM_T_REGS; i++) {
    if(opq->reg_t[i] < 32) {
      gen_op_move_t_gpr[i][opq->reg_t[i]](opq, 0);
      opq->reg_t[i] = 32;
    }
  }

  opq->xref = 1;

  return 1;
}

static void gen_j_imm(struct op_queue *opq, oraddr_t off)
{
  int jump_local;
  int i;
  int reg_t[NUM_T_REGS];

  off <<= 2;

  jump_local = find_jump_loc(opq->insn_addr + off, opq);

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_set_pc_delay_imm(opq, 1, off);
    gen_op_do_sched(opq, 1);
    return;
  }

  gen_op_set_delay_insn(opq, 1);
  gen_op_do_sched(opq, 1);

  /* Recompileing the delay slot instruction must see the temoraries being in
   * the state after the jump/branch instruction not before */
  memcpy(reg_t, opq->reg_t, sizeof(reg_t));
  memcpy(opq->reg_t, opq->reg_t_d, sizeof(reg_t));

  /* Generate the delay slot instruction */
  recompile_insn(opq, opq->insn_addr + 4, 1);

  memcpy(opq->reg_t, reg_t, sizeof(reg_t));

  ship_gprs_out_t(opq, 1, opq->reg_t_d);

  gen_op_add_pc(opq, 1, (orreg_t)off - 8);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_sched_delay(opq, 1);

  if(jump_local) {
    gen_op_jmp_imm(opq, 1, 0);
    opq->jump_local = opq->num_ops_param - 1;
    opq->jump_local_loc = (opq->insn_addr + (orreg_t)off) & (config.immu.pagesize - 1);
  } else
    gen_op_do_jump(opq, 1);
}

static const generic_gen_op set_pc_delay_gpr[32] = {
 NULL,
 gen_op_move_gpr1_pc_delay,
 gen_op_move_gpr2_pc_delay,
 gen_op_move_gpr3_pc_delay,
 gen_op_move_gpr4_pc_delay,
 gen_op_move_gpr5_pc_delay,
 gen_op_move_gpr6_pc_delay,
 gen_op_move_gpr7_pc_delay,
 gen_op_move_gpr8_pc_delay,
 gen_op_move_gpr9_pc_delay,
 gen_op_move_gpr10_pc_delay,
 gen_op_move_gpr11_pc_delay,
 gen_op_move_gpr12_pc_delay,
 gen_op_move_gpr13_pc_delay,
 gen_op_move_gpr14_pc_delay,
 gen_op_move_gpr15_pc_delay,
 gen_op_move_gpr16_pc_delay,
 gen_op_move_gpr17_pc_delay,
 gen_op_move_gpr18_pc_delay,
 gen_op_move_gpr19_pc_delay,
 gen_op_move_gpr20_pc_delay,
 gen_op_move_gpr21_pc_delay,
 gen_op_move_gpr22_pc_delay,
 gen_op_move_gpr23_pc_delay,
 gen_op_move_gpr24_pc_delay,
 gen_op_move_gpr25_pc_delay,
 gen_op_move_gpr26_pc_delay,
 gen_op_move_gpr27_pc_delay,
 gen_op_move_gpr28_pc_delay,
 gen_op_move_gpr29_pc_delay,
 gen_op_move_gpr30_pc_delay,
 gen_op_move_gpr31_pc_delay };

static void gen_j_reg(struct op_queue *opq, unsigned int gpr, int insn_index,
                      uint32_t insn)
{
  int i;
  int reg_t[NUM_T_REGS];

  if(do_stats)
    gen_op_analysis(opq, 1, insn_index, insn);

  if(!gpr)
    gen_op_clear_pc_delay(opq, 1);
  else
    set_pc_delay_gpr[gpr](opq, 1);

  gen_op_do_sched(opq, 1);

  /* Recompileing the delay slot instruction must see the temoraries being in
   * the state after the jump/branch instruction not before */
  memcpy(reg_t, opq->reg_t, sizeof(reg_t));
  memcpy(opq->reg_t, opq->reg_t_d, sizeof(reg_t));

  /* Generate the delay slot instruction */
  gen_op_set_delay_insn(opq, 1);
  recompile_insn(opq, opq->insn_addr + 4, 1);

  memcpy(opq->reg_t, reg_t, sizeof(reg_t));

  ship_gprs_out_t(opq, 1, opq->reg_t_d);

  gen_op_set_pc_pc_delay(opq, 1);
  gen_op_clear_delay_insn(opq, 1);
  gen_op_do_sched_delay(opq, 1);

  gen_op_do_jump_delay(opq, 1);
}

/*------------------------------[ Operation generation for an instruction ]---*/
/* FIXME: Flag setting is not done in any instruction */
/* FIXME: Since r0 is not moved into a temporary, check all arguments below! */

static const generic_gen_op clear_t[NUM_T_REGS] =
 { gen_op_clear_t0, gen_op_clear_t1, gen_op_clear_t2 };

static const generic_gen_op move_t_t[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { NULL, gen_op_move_t0_t1, gen_op_move_t0_t2 },
/* param0 -> t1 */ { gen_op_move_t1_t0, NULL, gen_op_move_t1_t2 },
/* param0 -> t2 */ { gen_op_move_t2_t0, gen_op_move_t2_t1, NULL } };

static const imm_gen_op mov_t_imm[NUM_T_REGS] =
 { gen_op_t0_imm, gen_op_t1_imm, gen_op_t2_imm };

static const imm_gen_op l_add_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_add_imm_t0_t0, gen_op_add_imm_t0_t1, gen_op_add_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_add_imm_t1_t0, gen_op_add_imm_t1_t1, gen_op_add_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_add_imm_t2_t0, gen_op_add_imm_t2_t1, gen_op_add_imm_t2_t2 } };

static const generic_gen_op l_add_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_add_t0_t0_t0, gen_op_add_t0_t0_t1, gen_op_add_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_add_t0_t1_t0, gen_op_add_t0_t1_t1, gen_op_add_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_add_t0_t2_t0, gen_op_add_t0_t2_t1, gen_op_add_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_add_t1_t0_t0, gen_op_add_t1_t0_t1, gen_op_add_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_add_t1_t1_t0, gen_op_add_t1_t1_t1, gen_op_add_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_add_t1_t2_t0, gen_op_add_t1_t2_t1, gen_op_add_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_add_t2_t0_t0, gen_op_add_t2_t0_t1, gen_op_add_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_add_t2_t1_t0, gen_op_add_t2_t1_t1, gen_op_add_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_add_t2_t2_t0, gen_op_add_t2_t2_t1, gen_op_add_t2_t2_t2 } } };

void gen_l_add(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    /* Screw this, the operation shall do nothing */
    return;

  if(!param[1] && !param[2]) {
    /* Just clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[2]) {
    if(param[0] != param[1])
      /* This just moves a register */
      move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    /* Check if we are moveing an immediate */
    if(param_t[2] == T_NONE) {
      /* Yep, an immediate */
      mov_t_imm[param_t[0]](opq, 1, param[2]);
      return;
    }
    /* Just another move */
    if(param[0] != param[2])
      move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  /* Ok, This _IS_ an add... */
  if(param_t[2] == T_NONE)
    /* immediate */
    l_add_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_add_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const generic_gen_op l_addc_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_addc_t0_t0_t0, gen_op_addc_t0_t0_t1, gen_op_addc_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_addc_t0_t1_t0, gen_op_addc_t0_t1_t1, gen_op_addc_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_addc_t0_t2_t0, gen_op_addc_t0_t2_t1, gen_op_addc_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_addc_t1_t0_t0, gen_op_addc_t1_t0_t1, gen_op_addc_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_addc_t1_t1_t0, gen_op_addc_t1_t1_t1, gen_op_addc_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_addc_t1_t2_t0, gen_op_addc_t1_t2_t1, gen_op_addc_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_addc_t2_t0_t0, gen_op_addc_t2_t0_t1, gen_op_addc_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_addc_t2_t1_t0, gen_op_addc_t2_t1_t1, gen_op_addc_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_addc_t2_t2_t0, gen_op_addc_t2_t2_t1, gen_op_addc_t2_t2_t2 } } };

void gen_l_addc(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[0])
    /* Screw this, the operation shall do nothing */
    return;

  /* FIXME: More optimisations !! (...and immediate...) */
  l_addc_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const imm_gen_op l_and_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_and_imm_t0_t0, gen_op_and_imm_t0_t1, gen_op_and_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_and_imm_t1_t0, gen_op_and_imm_t1_t1, gen_op_and_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_and_imm_t2_t0, gen_op_and_imm_t2_t1, gen_op_and_imm_t2_t2 } };

static const generic_gen_op l_and_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { NULL, gen_op_and_t0_t0_t1, gen_op_and_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_and_t0_t1_t0, gen_op_and_t0_t1_t1, gen_op_and_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_and_t0_t2_t0, gen_op_and_t0_t2_t1, gen_op_and_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_and_t1_t0_t0, gen_op_and_t1_t0_t1, gen_op_and_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_and_t1_t1_t0, NULL, gen_op_and_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_and_t1_t2_t0, gen_op_and_t1_t2_t1, gen_op_and_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_and_t2_t0_t0, gen_op_and_t2_t0_t1, gen_op_and_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_and_t2_t1_t0, gen_op_and_t2_t1_t1, gen_op_and_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_and_t2_t2_t0, gen_op_and_t2_t2_t1, NULL } } };

void gen_l_and(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    /* Screw this, the operation shall do nothing */
    return;

  if(!param[1] || !param[2]) {
    /* Just clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if((param[0] == param[1] == param[2]) && (param_t[2] != T_NONE))
    return;

  if(param_t[2] == T_NONE)
    l_and_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_and_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_bf(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  int i;
  if(do_stats)
    gen_op_analysis(opq, 1, 3, 0x10000000 | (param[0] & 0x03ffffff));

  /* The temporaries are expected to be shiped out after the execution of the
   * branch instruction wether it branched or not */
  if(opq->prev) {
    ship_gprs_out_t(opq->prev, 1, opq->reg_t);
    for(i = 0; i < NUM_T_REGS; i++) {
      opq->reg_t[i] = 32;
      opq->reg_t_d[i] = 32;
    }
  }

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_check_flag_delay(opq, 1, param[0] << 2);
    gen_op_do_sched(opq, 1);
    opq->not_jump_loc = -1;
    return;
  }
 
  gen_op_check_flag(opq, 1, 0);
  opq->not_jump_loc = opq->num_ops_param - 1;

  gen_j_imm(opq, param[0]);
}

void gen_l_bnf(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  int i;
  if(do_stats)
    gen_op_analysis(opq, 1, 2, 0x0c000000 | (param[0] & 0x03ffffff));

  /* The temporaries are expected to be shiped out after the execution of the
   * branch instruction wether it branched or not */
  if(opq->prev) {
    ship_gprs_out_t(opq->prev, 1, opq->reg_t);
    for(i = 0; i < NUM_T_REGS; i++) {
      opq->reg_t[i] = 32;
      opq->reg_t_d[i] = 32;
    }
  }

  if(IADDR_PAGE(opq->insn_addr) != IADDR_PAGE(opq->insn_addr + 4)) {
    gen_op_check_not_flag_delay(opq, 1, param[0] << 2);
    gen_op_do_sched(opq, 1);
    opq->not_jump_loc = -1;
    return;
  }

  gen_op_check_not_flag(opq, 1, 0);
  opq->not_jump_loc = opq->num_ops_param - 1;

  gen_j_imm(opq, param[0]);

  /* The temporaries don't get shiped out if the branch is not taken */
  memcpy(opq->next->reg_t, opq->reg_t, sizeof(opq->reg_t));
}

static const generic_gen_op l_cmov_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { NULL, gen_op_cmov_t0_t0_t1, gen_op_cmov_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_cmov_t0_t1_t0, NULL, gen_op_cmov_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_cmov_t0_t2_t0, gen_op_cmov_t0_t2_t1, NULL } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { NULL, gen_op_cmov_t1_t0_t1, gen_op_cmov_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_cmov_t1_t1_t0, NULL, gen_op_cmov_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_cmov_t1_t2_t0, gen_op_cmov_t1_t2_t1, NULL } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { NULL, gen_op_cmov_t2_t0_t1, gen_op_cmov_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_cmov_t2_t1_t0, NULL, gen_op_cmov_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_cmov_t2_t2_t0, gen_op_cmov_t2_t2_t1, NULL } } };

/* FIXME: Check if either opperand 1 or 2 is r0 */
void gen_l_cmov(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1] && !param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(param[1] == param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if((param[1] == param[2]) && (param[0] == param[1]))
    return;

  l_cmov_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_cust1(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust2(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust3(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust4(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust5(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust6(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust7(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

void gen_l_cust8(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
}

/* FIXME: All registers need to be stored before the div instructions as they
 * have the potenticial to cause an exception */

static const generic_gen_op check_null_excpt[NUM_T_REGS] =
 { gen_op_check_null_except_t0, gen_op_check_null_except_t1, gen_op_check_null_except_t2 };

static const generic_gen_op check_null_excpt_delay[NUM_T_REGS] = {
 gen_op_check_null_except_t0_delay,
 gen_op_check_null_except_t1_delay,
 gen_op_check_null_except_t2_delay };

static const generic_gen_op l_div_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_div_t0_t0_t0, gen_op_div_t0_t0_t1, gen_op_div_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_div_t0_t1_t0, gen_op_div_t0_t1_t1, gen_op_div_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_div_t0_t2_t0, gen_op_div_t0_t2_t1, gen_op_div_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_div_t1_t0_t0, gen_op_div_t1_t0_t1, gen_op_div_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_div_t1_t1_t0, gen_op_div_t1_t1_t1, gen_op_div_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_div_t1_t2_t0, gen_op_div_t1_t2_t1, gen_op_div_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_div_t2_t0_t0, gen_op_div_t2_t0_t1, gen_op_div_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_div_t2_t1_t0, gen_op_div_t2_t1_t1, gen_op_div_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_div_t2_t2_t0, gen_op_div_t2_t2_t1, gen_op_div_t2_t2_t2 } } };

void gen_l_div(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[2]) {
    /* There is no option.  This _will_ cause an illeagal exception */
    if(!delay_slot)
      gen_op_illegal(opq, 1);
    else
      gen_op_illegal(opq, 1);
    return;
  }

  if(!delay_slot)
    check_null_excpt[param_t[2]](opq, 1);
  else
    check_null_excpt_delay[param_t[2]](opq, 1);

  if(!param[0])
    return;

  if(!param[1]) {
    /* Clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }
    
  l_div_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const generic_gen_op l_divu_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_divu_t0_t0_t0, gen_op_divu_t0_t0_t1, gen_op_divu_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_divu_t0_t1_t0, gen_op_divu_t0_t1_t1, gen_op_divu_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_divu_t0_t2_t0, gen_op_divu_t0_t2_t1, gen_op_divu_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_divu_t1_t0_t0, gen_op_divu_t1_t0_t1, gen_op_divu_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_divu_t1_t1_t0, gen_op_divu_t1_t1_t1, gen_op_divu_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_divu_t1_t2_t0, gen_op_divu_t1_t2_t1, gen_op_divu_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_divu_t2_t0_t0, gen_op_divu_t2_t0_t1, gen_op_divu_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_divu_t2_t1_t0, gen_op_divu_t2_t1_t1, gen_op_divu_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_divu_t2_t2_t0, gen_op_divu_t2_t2_t1, gen_op_divu_t2_t2_t2 } } };

void gen_l_divu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[2]) {
    /* There is no option.  This _will_ cause an illeagal exception */
    if(!delay_slot)
      gen_op_illegal(opq, 1);
    else
      gen_op_illegal(opq, 1);
    return;
  }

  if(!delay_slot)
    check_null_excpt[param_t[2]](opq, 1);
  else
    check_null_excpt_delay[param_t[2]](opq, 1);

  if(!param[0])
    return;

  if(!param[1]) {
    /* Clear param_t[0] */
    clear_t[param_t[0]](opq, 1);
    return;
  }
 
  l_divu_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const generic_gen_op l_extbs_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_extbs_t0_t0, gen_op_extbs_t0_t1, gen_op_extbs_t0_t2 },
/* param0 -> t1 */ { gen_op_extbs_t1_t0, gen_op_extbs_t1_t1, gen_op_extbs_t1_t2 },
/* param0 -> t2 */ { gen_op_extbs_t2_t0, gen_op_extbs_t2_t1, gen_op_extbs_t2_t2 } };

void gen_l_extbs(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_extbs_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_extbz_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_extbz_t0_t0, gen_op_extbz_t0_t1, gen_op_extbz_t0_t2 },
/* param0 -> t1 */ { gen_op_extbz_t1_t0, gen_op_extbz_t1_t1, gen_op_extbz_t1_t2 },
/* param0 -> t2 */ { gen_op_extbz_t2_t0, gen_op_extbz_t2_t1, gen_op_extbz_t2_t2 } };

void gen_l_extbz(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_extbz_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_exths_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_exths_t0_t0, gen_op_exths_t0_t1, gen_op_exths_t0_t2 },
/* param0 -> t1 */ { gen_op_exths_t1_t0, gen_op_exths_t1_t1, gen_op_exths_t1_t2 },
/* param0 -> t2 */ { gen_op_exths_t2_t0, gen_op_exths_t2_t1, gen_op_exths_t2_t2 } };

void gen_l_exths(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_exths_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_exthz_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_exthz_t0_t0, gen_op_exthz_t0_t1, gen_op_exthz_t0_t2 },
/* param0 -> t1 */ { gen_op_exthz_t1_t0, gen_op_exthz_t1_t1, gen_op_exthz_t1_t2 },
/* param0 -> t2 */ { gen_op_exthz_t2_t0, gen_op_exthz_t2_t1, gen_op_exthz_t2_t2 } };

void gen_l_exthz(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_exthz_t_table[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_extws(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(param[0] == param[1])
    return;

  /* In the 32-bit architechture this instruction reduces to a move */
  move_t_t[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_extwz(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(param[0] == param[1])
    return;

  /* In the 32-bit architechture this instruction reduces to a move */
  move_t_t[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_ff1_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_ff1_t0_t0, gen_op_ff1_t0_t1, gen_op_ff1_t0_t2 },
/* param0 -> t1 */ { gen_op_ff1_t1_t0, gen_op_ff1_t1_t1, gen_op_ff1_t1_t2 },
/* param0 -> t2 */ { gen_op_ff1_t2_t0, gen_op_ff1_t2_t1, gen_op_ff1_t2_t2 } };

void gen_l_ff1(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_ff1_t_table[param_t[0]][param_t[1]](opq, 1);
}

void gen_l_j(struct op_queue *opq, int param_t[3], orreg_t param[3],
             int delay_slot)
{
  if(do_stats)
    gen_op_analysis(opq, 1, 0, param[0] & 0x03ffffff);

  gen_j_imm(opq, param[0]);
}

void gen_l_jal(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  /* Store the return address */
  gen_op_store_link_addr_gpr(opq, 1);

  if(do_stats)
    gen_op_analysis(opq, 1, 1, 0x04000000 | (param[0] & 0x03ffffff));

  gen_j_imm(opq, param[0]);
}

void gen_l_jr(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  gen_j_reg(opq, param[0], 104, 0x14000000 | (param[0] << 11));
}

void gen_l_jalr(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  /* Store the return address */
  gen_op_store_link_addr_gpr(opq, 1);

  gen_j_reg(opq, param[0], 105, 0x18000000 | (param[0] << 11));
}

/* FIXME: Optimise all load instruction when the disposition == 0 */

static const imm_gen_op l_lbs_imm_t_table[NUM_T_REGS] =
 { gen_op_lbs_imm_t0, gen_op_lbs_imm_t1, gen_op_lbs_imm_t2 };

static const imm_gen_op l_lbs_t_table[3][3] = {
/* param0 -> t0 */ { gen_op_lbs_t0_t0, gen_op_lbs_t0_t1, gen_op_lbs_t0_t2 },
/* param0 -> t1 */ { gen_op_lbs_t1_t0, gen_op_lbs_t1_t1, gen_op_lbs_t1_t2 },
/* param0 -> t2 */ { gen_op_lbs_t2_t0, gen_op_lbs_t2_t1, gen_op_lbs_t2_t2 } };

void gen_l_lbs(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lbs_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lbs_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_lbz_imm_t_table[NUM_T_REGS] =
 { gen_op_lbz_imm_t0, gen_op_lbz_imm_t1, gen_op_lbz_imm_t2 };

static const imm_gen_op l_lbz_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_lbz_t0_t0, gen_op_lbz_t0_t1, gen_op_lbz_t0_t2 },
/* param0 -> t1 */ { gen_op_lbz_t1_t0, gen_op_lbz_t1_t1, gen_op_lbz_t1_t2 },
/* param0 -> t2 */ { gen_op_lbz_t2_t0, gen_op_lbz_t2_t1, gen_op_lbz_t2_t2 } };

void gen_l_lbz(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lbz_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lbz_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_lhs_imm_t_table[NUM_T_REGS] =
 { gen_op_lhs_imm_t0, gen_op_lhs_imm_t1, gen_op_lhs_imm_t2 };

static const imm_gen_op l_lhs_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_lhs_t0_t0, gen_op_lhs_t0_t1, gen_op_lhs_t0_t2 },
/* param0 -> t1 */ { gen_op_lhs_t1_t0, gen_op_lhs_t1_t1, gen_op_lhs_t1_t2 },
/* param0 -> t2 */ { gen_op_lhs_t2_t0, gen_op_lhs_t2_t1, gen_op_lhs_t2_t2 } };

void gen_l_lhs(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lhs_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lhs_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_lhz_imm_t_table[NUM_T_REGS] =
 { gen_op_lhz_imm_t0, gen_op_lhz_imm_t1, gen_op_lhz_imm_t2 };

static const imm_gen_op l_lhz_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_lhz_t0_t0, gen_op_lhz_t0_t1, gen_op_lhz_t0_t2 },
/* param0 -> t1 */ { gen_op_lhz_t1_t0, gen_op_lhz_t1_t1, gen_op_lhz_t1_t2 },
/* param0 -> t2 */ { gen_op_lhz_t2_t0, gen_op_lhz_t2_t1, gen_op_lhz_t2_t2 } };

void gen_l_lhz(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lhz_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lhz_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_lws_imm_t_table[NUM_T_REGS] =
 { gen_op_lws_imm_t0, gen_op_lws_imm_t1, gen_op_lws_imm_t2 };

static const imm_gen_op l_lws_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_lws_t0_t0, gen_op_lws_t0_t1, gen_op_lws_t0_t2 },
/* param0 -> t1 */ { gen_op_lws_t1_t0, gen_op_lws_t1_t1, gen_op_lws_t1_t2 },
/* param0 -> t2 */ { gen_op_lws_t2_t0, gen_op_lws_t2_t1, gen_op_lws_t2_t2 } };

void gen_l_lws(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lws_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lws_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_lwz_imm_t_table[NUM_T_REGS] =
 { gen_op_lwz_imm_t0, gen_op_lwz_imm_t1, gen_op_lwz_imm_t2 };

static const imm_gen_op l_lwz_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_lwz_t0_t0, gen_op_lwz_t0_t1, gen_op_lwz_t0_t2 },
/* param0 -> t1 */ { gen_op_lwz_t1_t0, gen_op_lwz_t1_t1, gen_op_lwz_t1_t2 },
/* param0 -> t2 */ { gen_op_lwz_t2_t0, gen_op_lwz_t2_t1, gen_op_lwz_t2_t2 } };

void gen_l_lwz(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0]) {
    /* FIXME: This will work, but the statistics need to be updated... */
    return;
  }

  if(!param[2]) {
    /* Load the data from the immediate */
    l_lwz_imm_t_table[param_t[0]](opq, 1, param[1]);
    return;
  }

  l_lwz_t_table[param_t[0]][param_t[2]](opq, 1, param[1]);
}

static const imm_gen_op l_mac_imm_t_table[NUM_T_REGS] =
 { gen_op_mac_imm_t0, gen_op_mac_imm_t1, gen_op_mac_imm_t2 };

static const generic_gen_op l_mac_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_mac_t0_t0, gen_op_mac_t0_t1, gen_op_mac_t0_t2 },
/* param0 -> t1 */ { gen_op_mac_t0_t1, gen_op_mac_t1_t1, gen_op_mac_t1_t2 },
/* param0 -> t2 */ { gen_op_mac_t0_t2, gen_op_mac_t1_t2, gen_op_mac_t2_t2 } };

void gen_l_mac(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0] || !param[1])
    return;

  if(param_t[1] == T_NONE)
    l_mac_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_mac_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_macrc_t_table[NUM_T_REGS] =
 { gen_op_macrc_t0, gen_op_macrc_t1, gen_op_macrc_t2 };

void gen_l_macrc(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0]) {
    gen_op_macc(opq, 1);
    return;
  }

  l_macrc_t_table[param_t[0]](opq, 1);
}

static const imm_gen_op l_mfspr_imm_t_table[NUM_T_REGS] =
 { gen_op_mfspr_t0_imm, gen_op_mfspr_t1_imm, gen_op_mfspr_t2_imm };

static const imm_gen_op l_mfspr_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_mfspr_t0_t0, gen_op_mfspr_t0_t1, gen_op_mfspr_t0_t2 },
/* param0 -> t1 */ { gen_op_mfspr_t1_t0, gen_op_mfspr_t1_t1, gen_op_mfspr_t1_t2 },
/* param0 -> t2 */ { gen_op_mfspr_t2_t0, gen_op_mfspr_t2_t1, gen_op_mfspr_t2_t2 } };

void gen_l_mfspr(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    l_mfspr_imm_t_table[param_t[0]](opq, 1, param[2]);
    return;
  }

  l_mfspr_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
}

void gen_l_movhi(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  mov_t_imm[param_t[0]](opq, 1, param[1] << 16);
}

static const generic_gen_op l_msb_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_msb_t0_t0, gen_op_msb_t0_t1, gen_op_msb_t0_t2 },
/* param0 -> t1 */ { gen_op_msb_t0_t1, gen_op_msb_t1_t1, gen_op_msb_t1_t2 },
/* param0 -> t2 */ { gen_op_msb_t0_t2, gen_op_msb_t1_t2, gen_op_msb_t2_t2 } };

void gen_l_msb(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0] || !param[1])
    return;

  l_msb_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_mtspr_clear_t_table[NUM_T_REGS] =
 { gen_op_mtspr_t0_clear, gen_op_mtspr_t1_clear, gen_op_mtspr_t2_clear };

static const imm_gen_op l_mtspr_imm_t_table[NUM_T_REGS] =
 { gen_op_mtspr_imm_t0, gen_op_mtspr_imm_t1, gen_op_mtspr_imm_t2 };

static const imm_gen_op l_mtspr_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_mtspr_t0_t0, gen_op_mtspr_t0_t1, gen_op_mtspr_t0_t2 },
/* param0 -> t1 */ { gen_op_mtspr_t1_t0, gen_op_mtspr_t1_t1, gen_op_mtspr_t1_t2 },
/* param0 -> t2 */ { gen_op_mtspr_t2_t0, gen_op_mtspr_t2_t1, gen_op_mtspr_t2_t2 } };

void gen_l_mtspr(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0]) {
    if(!param[1]) {
      /* Clear the immediate SPR */
      gen_op_mtspr_imm_clear(opq, 1, param[2]);
      return;
    }
    l_mtspr_imm_t_table[param_t[1]](opq, 1, param[2]);
    return;
  }

  if(!param[1]) {
    l_mtspr_clear_t_table[param_t[0]](opq, 1, param[2]);
    return;
  }

  l_mtspr_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
}

static const imm_gen_op l_mul_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_mul_imm_t0_t0, gen_op_mul_imm_t0_t1, gen_op_mul_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_mul_imm_t1_t0, gen_op_mul_imm_t1_t1, gen_op_mul_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_mul_imm_t2_t0, gen_op_mul_imm_t2_t1, gen_op_mul_imm_t2_t2 } };

static const generic_gen_op l_mul_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_mul_t0_t0_t0, gen_op_mul_t0_t0_t1, gen_op_mul_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_mul_t0_t1_t0, gen_op_mul_t0_t1_t1, gen_op_mul_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_mul_t0_t2_t0, gen_op_mul_t0_t2_t1, gen_op_mul_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_mul_t1_t0_t0, gen_op_mul_t1_t0_t1, gen_op_mul_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_mul_t1_t1_t0, gen_op_mul_t1_t1_t1, gen_op_mul_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_mul_t1_t2_t0, gen_op_mul_t1_t2_t1, gen_op_mul_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_mul_t2_t0_t0, gen_op_mul_t2_t0_t1, gen_op_mul_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_mul_t2_t1_t0, gen_op_mul_t2_t1_t1, gen_op_mul_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_mul_t2_t2_t0, gen_op_mul_t2_t2_t1, gen_op_mul_t2_t2_t2 } } };

void gen_l_mul(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1] || !param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_mul_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_mul_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const generic_gen_op l_mulu_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_mulu_t0_t0_t0, gen_op_mulu_t0_t0_t1, gen_op_mulu_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_mulu_t0_t1_t0, gen_op_mulu_t0_t1_t1, gen_op_mulu_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_mulu_t0_t2_t0, gen_op_mulu_t0_t2_t1, gen_op_mulu_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_mulu_t1_t0_t0, gen_op_mulu_t1_t0_t1, gen_op_mulu_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_mulu_t1_t1_t0, gen_op_mulu_t1_t1_t1, gen_op_mulu_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_mulu_t1_t2_t0, gen_op_mulu_t1_t2_t1, gen_op_mulu_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_mulu_t2_t0_t0, gen_op_mulu_t2_t0_t1, gen_op_mulu_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_mulu_t2_t1_t0, gen_op_mulu_t2_t1_t1, gen_op_mulu_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_mulu_t2_t2_t0, gen_op_mulu_t2_t2_t1, gen_op_mulu_t2_t2_t2 } } };

void gen_l_mulu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1] || !param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  l_mulu_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_nop(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  /* Do parameter switch now */
  switch (param[0]) {
  case NOP_NOP:
    break;
  case NOP_EXIT:
    gen_op_nop_exit(opq, 1);
    break;
  case NOP_CNT_RESET:
    /* FIXME: Since op_nop_reset calls handle_except, this instruction wont show
     * up in the execution log, nor will the scheduler run */
    gen_op_nop_reset(opq, 1);
    break;    
  case NOP_PRINTF:
    gen_op_nop_printf(opq, 1);
    break;
  case NOP_REPORT:
    gen_op_nop_report(opq, 1);
    break;
  default:
    if((param[0] >= NOP_REPORT_FIRST) && (param[0] <= NOP_REPORT_LAST))
      gen_op_nop_report_imm(opq, 1, param[0] - NOP_REPORT_FIRST);
    break;
  }
}

static const imm_gen_op l_or_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_or_imm_t0_t0, gen_op_or_imm_t0_t1, gen_op_or_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_or_imm_t1_t0, gen_op_or_imm_t1_t1, gen_op_or_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_or_imm_t2_t0, gen_op_or_imm_t2_t1, gen_op_or_imm_t2_t2 } };

static const generic_gen_op l_or_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { NULL, gen_op_or_t0_t0_t1, gen_op_or_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_or_t0_t1_t0, gen_op_or_t0_t1_t1, gen_op_or_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_or_t0_t2_t0, gen_op_or_t0_t2_t1, gen_op_or_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_or_t1_t0_t0, gen_op_or_t1_t0_t1, gen_op_or_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_or_t1_t1_t0, NULL, gen_op_or_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_or_t1_t2_t0, gen_op_or_t1_t2_t1, gen_op_or_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_or_t2_t0_t0, gen_op_or_t2_t0_t1, gen_op_or_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_or_t2_t1_t0, gen_op_or_t2_t1_t1, gen_op_or_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_or_t2_t2_t0, gen_op_or_t2_t2_t1, NULL } } };

void gen_l_or(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  if(!param[0])
    return;

  if((param[0] == param[1] == param[2]) && (param_t[2] != T_NONE))
    return;

  if(!param[1] && !param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[2]) {
    if((param_t[2] == T_NONE) && (param[0] == param[1]))
      return;
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    /* Check if we are moveing an immediate */
    if(param_t[2] == T_NONE) {
      /* Yep, an immediate */
      mov_t_imm[param_t[0]](opq, 1, param[2]);
      return;
    }
    /* Just another move */
    move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_or_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_or_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_rfe(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(do_stats)
    gen_op_analysis(opq, 1, 12, 0x24000000);

  gen_op_prep_rfe(opq, 1);
  gen_op_do_sched(opq, 1);
  gen_op_do_jump(opq, 1);
}

/* FIXME: All store instructions should be optimised when the disposition = 0 */

static const imm_gen_op l_sb_clear_table[NUM_T_REGS] =
 { gen_op_sb_clear_t0, gen_op_sb_clear_t1, gen_op_sb_clear_t2 };

static const imm_gen_op l_sb_imm_t_table[NUM_T_REGS] =
 { gen_op_sb_imm_t0, gen_op_sb_imm_t1, gen_op_sb_imm_t2 };

static const imm_gen_op l_sb_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sb_t0_t0, gen_op_sb_t0_t1, gen_op_sb_t0_t2 },
/* param0 -> t1 */ { gen_op_sb_t1_t0, gen_op_sb_t1_t1, gen_op_sb_t1_t2 },
/* param0 -> t2 */ { gen_op_sb_t2_t0, gen_op_sb_t2_t1, gen_op_sb_t2_t2 } };

void gen_l_sb(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  if(!param[2]) {
    if(!param[1]) {
      gen_op_sb_clear_imm(opq, 1, param[0]);
      return;
    }
    l_sb_clear_table[param_t[1]](opq, 1, param[0]);
    return;
  }

  if(!param[1]) {
    /* Store the data to the immediate */
    l_sb_imm_t_table[param_t[2]](opq, 1, param[0]);
    return;
  }

  l_sb_t_table[param_t[1]][param_t[2]](opq, 1, param[0]);
}

static const imm_gen_op l_sh_clear_table[NUM_T_REGS] =
 { gen_op_sh_clear_t0, gen_op_sh_clear_t1, gen_op_sh_clear_t2 };

static const imm_gen_op l_sh_imm_t_table[NUM_T_REGS] =
 { gen_op_sh_imm_t0, gen_op_sh_imm_t1, gen_op_sh_imm_t2 };

static const imm_gen_op l_sh_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sh_t0_t0, gen_op_sh_t0_t1, gen_op_sh_t0_t2 },
/* param0 -> t1 */ { gen_op_sh_t1_t0, gen_op_sh_t1_t1, gen_op_sh_t1_t2 },
/* param0 -> t2 */ { gen_op_sh_t2_t0, gen_op_sh_t2_t1, gen_op_sh_t2_t2 } };

void gen_l_sh(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  if(!param[2]) {
    if(!param[1]) {
      gen_op_sh_clear_imm(opq, 1, param[0]);
      return;
    }
    l_sh_clear_table[param_t[1]](opq, 1, param[0]);
    return;
  }

  if(!param[1]) {
    /* Store the data to the immediate */
    l_sh_imm_t_table[param_t[2]](opq, 1, param[0]);
    return;
  }

  l_sh_t_table[param_t[1]][param_t[2]](opq, 1, param[0]);
}

static const imm_gen_op l_sw_clear_table[NUM_T_REGS] =
 { gen_op_sw_clear_t0, gen_op_sw_clear_t1, gen_op_sw_clear_t2 };

static const imm_gen_op l_sw_imm_t_table[NUM_T_REGS] =
 { gen_op_sw_imm_t0, gen_op_sw_imm_t1, gen_op_sw_imm_t2 };

static const imm_gen_op l_sw_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sw_t0_t0, gen_op_sw_t0_t1, gen_op_sw_t0_t2 },
/* param0 -> t1 */ { gen_op_sw_t1_t0, gen_op_sw_t1_t1, gen_op_sw_t1_t2 },
/* param0 -> t2 */ { gen_op_sw_t2_t0, gen_op_sw_t2_t1, gen_op_sw_t2_t2 } };

void gen_l_sw(struct op_queue *opq, int param_t[3], orreg_t param[3],
              int delay_slot)
{
  if(!param[2]) {
    if(!param[1]) {
      gen_op_sw_clear_imm(opq, 1, param[0]);
      return;
    }
    l_sw_clear_table[param_t[1]](opq, 1, param[0]);
    return;
  }

  if(!param[1]) {
    /* Store the data to the immediate */
    l_sw_imm_t_table[param_t[2]](opq, 1, param[0]);
    return;
  }

  l_sw_t_table[param_t[1]][param_t[2]](opq, 1, param[0]);
}

static const generic_gen_op l_sfeq_null_t_table[NUM_T_REGS] =
 { gen_op_sfeq_null_t0, gen_op_sfeq_null_t1, gen_op_sfeq_null_t2 };

static const imm_gen_op l_sfeq_imm_t_table[NUM_T_REGS] =
 { gen_op_sfeq_imm_t0, gen_op_sfeq_imm_t1, gen_op_sfeq_imm_t2 };

static const generic_gen_op l_sfeq_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfeq_t0_t0, gen_op_sfeq_t0_t1, gen_op_sfeq_t0_t2 },
/* param0 -> t1 */ { gen_op_sfeq_t1_t0, gen_op_sfeq_t1_t1, gen_op_sfeq_t1_t2 },
/* param0 -> t2 */ { gen_op_sfeq_t2_t0, gen_op_sfeq_t2_t1, gen_op_sfeq_t2_t2 } };

void gen_l_sfeq(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    if(param_t[1] == T_NONE) {
      if(!param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfeq_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfeq_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfeq_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfeq_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_sfges_null_t_table[NUM_T_REGS] =
 { gen_op_sfges_null_t0, gen_op_sfges_null_t1, gen_op_sfges_null_t2 };

static const generic_gen_op l_sfles_null_t_table[NUM_T_REGS] =
 { gen_op_sfles_null_t0, gen_op_sfles_null_t1, gen_op_sfles_null_t2 };

static const imm_gen_op l_sfges_imm_t_table[NUM_T_REGS] =
 { gen_op_sfges_imm_t0, gen_op_sfges_imm_t1, gen_op_sfges_imm_t2 };

static const generic_gen_op l_sfges_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfges_t0_t0, gen_op_sfges_t0_t1, gen_op_sfges_t0_t2 },
/* param0 -> t1 */ { gen_op_sfges_t1_t0, gen_op_sfges_t1_t1, gen_op_sfges_t1_t2 },
/* param0 -> t2 */ { gen_op_sfges_t2_t0, gen_op_sfges_t2_t1, gen_op_sfges_t2_t2 } };

void gen_l_sfges(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfles IS correct */
    if(param_t[1] == T_NONE) {
      if(0 >= (orreg_t)param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfles_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfges_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfges_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfges_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_sfgeu_null_t_table[NUM_T_REGS] =
 { gen_op_sfgeu_null_t0, gen_op_sfgeu_null_t1, gen_op_sfgeu_null_t2 };

static const generic_gen_op l_sfleu_null_t_table[NUM_T_REGS] =
 { gen_op_sfleu_null_t0, gen_op_sfleu_null_t1, gen_op_sfleu_null_t2 };

static const imm_gen_op l_sfgeu_imm_t_table[NUM_T_REGS] =
 { gen_op_sfgeu_imm_t0, gen_op_sfgeu_imm_t1, gen_op_sfgeu_imm_t2 };

static const generic_gen_op l_sfgeu_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfgeu_t0_t0, gen_op_sfgeu_t0_t1, gen_op_sfgeu_t0_t2 },
/* param0 -> t1 */ { gen_op_sfgeu_t1_t0, gen_op_sfgeu_t1_t1, gen_op_sfgeu_t1_t2 },
/* param0 -> t2 */ { gen_op_sfgeu_t2_t0, gen_op_sfgeu_t2_t1, gen_op_sfgeu_t2_t2 } };

void gen_l_sfgeu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfleu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 >= param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfleu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfgeu_null_t_table[param_t[0]](opq, 1);
    return;
  }
  if(param_t[1] == T_NONE)
    l_sfgeu_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfgeu_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_sfgts_null_t_table[NUM_T_REGS] =
 { gen_op_sfgts_null_t0, gen_op_sfgts_null_t1, gen_op_sfgts_null_t2 };

static const generic_gen_op l_sflts_null_t_table[NUM_T_REGS] =
 { gen_op_sflts_null_t0, gen_op_sflts_null_t1, gen_op_sflts_null_t2 };

static const imm_gen_op l_sfgts_imm_t_table[NUM_T_REGS] =
 { gen_op_sfgts_imm_t0, gen_op_sfgts_imm_t1, gen_op_sfgts_imm_t2 };

static const generic_gen_op l_sfgts_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfgts_t0_t0, gen_op_sfgts_t0_t1, gen_op_sfgts_t0_t2 },
/* param0 -> t1 */ { gen_op_sfgts_t1_t0, gen_op_sfgts_t1_t1, gen_op_sfgts_t1_t2 },
/* param0 -> t2 */ { gen_op_sfgts_t2_t0, gen_op_sfgts_t2_t1, gen_op_sfgts_t2_t2 } };

void gen_l_sfgts(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sflts IS correct */
    if(param_t[1] == T_NONE) {
      if(0 > (orreg_t)param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sflts_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfgts_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfgts_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfgts_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_sfgtu_null_t_table[NUM_T_REGS] =
 { gen_op_sfgtu_null_t0, gen_op_sfgtu_null_t1, gen_op_sfgtu_null_t2 };

static const generic_gen_op l_sfltu_null_t_table[NUM_T_REGS] =
 { gen_op_sfltu_null_t0, gen_op_sfltu_null_t1, gen_op_sfltu_null_t2 };

static const imm_gen_op l_sfgtu_imm_t_table[NUM_T_REGS] =
 { gen_op_sfgtu_imm_t0, gen_op_sfgtu_imm_t1, gen_op_sfgtu_imm_t2 };

static const generic_gen_op l_sfgtu_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfgtu_t0_t0, gen_op_sfgtu_t0_t1, gen_op_sfgtu_t0_t2 },
/* param0 -> t1 */ { gen_op_sfgtu_t1_t0, gen_op_sfgtu_t1_t1, gen_op_sfgtu_t1_t2 },
/* param0 -> t2 */ { gen_op_sfgtu_t2_t0, gen_op_sfgtu_t2_t1, gen_op_sfgtu_t2_t2 } };

void gen_l_sfgtu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfltu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 > param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfltu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfgtu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfgtu_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfgtu_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_sfles_imm_t_table[NUM_T_REGS] =
 { gen_op_sfles_imm_t0, gen_op_sfles_imm_t1, gen_op_sfles_imm_t2 };

static const generic_gen_op l_sfles_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfles_t0_t0, gen_op_sfles_t0_t1, gen_op_sfles_t0_t2 },
/* param0 -> t1 */ { gen_op_sfles_t1_t0, gen_op_sfles_t1_t1, gen_op_sfles_t1_t2 },
/* param0 -> t2 */ { gen_op_sfles_t2_t0, gen_op_sfles_t2_t1, gen_op_sfles_t2_t2 } };

void gen_l_sfles(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfges IS correct */
    if(param_t[1] == T_NONE) {
      if(0 <= (orreg_t)param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfges_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfles_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfles_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfles_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_sfleu_imm_t_table[NUM_T_REGS] =
 { gen_op_sfleu_imm_t0, gen_op_sfleu_imm_t1, gen_op_sfleu_imm_t2 };

static const generic_gen_op l_sfleu_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfleu_t0_t0, gen_op_sfleu_t0_t1, gen_op_sfleu_t0_t2 },
/* param0 -> t1 */ { gen_op_sfleu_t1_t0, gen_op_sfleu_t1_t1, gen_op_sfleu_t1_t2 },
/* param0 -> t2 */ { gen_op_sfleu_t2_t0, gen_op_sfleu_t2_t1, gen_op_sfleu_t2_t2 } };

void gen_l_sfleu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfleu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 <= param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgeu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfleu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfleu_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfleu_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_sflts_imm_t_table[NUM_T_REGS] =
 { gen_op_sflts_imm_t0, gen_op_sflts_imm_t1, gen_op_sflts_imm_t2 };

static const generic_gen_op l_sflts_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sflts_t0_t0, gen_op_sflts_t0_t1, gen_op_sflts_t0_t2 },
/* param0 -> t1 */ { gen_op_sflts_t1_t0, gen_op_sflts_t1_t1, gen_op_sflts_t1_t2 },
/* param0 -> t2 */ { gen_op_sflts_t2_t0, gen_op_sflts_t2_t1, gen_op_sflts_t2_t2 } };

void gen_l_sflts(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfgts IS correct */
    if(param_t[1] == T_NONE) {
      if(0 < (orreg_t)param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgts_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sflts_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sflts_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sflts_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_sfltu_imm_t_table[NUM_T_REGS] =
 { gen_op_sfltu_imm_t0, gen_op_sfltu_imm_t1, gen_op_sfltu_imm_t2 };

static const generic_gen_op l_sfltu_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfltu_t0_t0, gen_op_sfltu_t0_t1, gen_op_sfltu_t0_t2 },
/* param0 -> t1 */ { gen_op_sfltu_t1_t0, gen_op_sfltu_t1_t1, gen_op_sfltu_t1_t2 },
/* param0 -> t2 */ { gen_op_sfltu_t2_t0, gen_op_sfltu_t2_t1, gen_op_sfltu_t2_t2 } };

void gen_l_sfltu(struct op_queue *opq, int param_t[3], orreg_t param[3],
                 int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_clear_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    /* sfgtu IS correct */
    if(param_t[1] == T_NONE) {
      if(0 < param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    } else
      l_sfgtu_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfltu_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfltu_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfltu_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const generic_gen_op l_sfne_null_t_table[NUM_T_REGS] =
 { gen_op_sfne_null_t0, gen_op_sfne_null_t1, gen_op_sfne_null_t2 };

static const imm_gen_op l_sfne_imm_t_table[NUM_T_REGS] =
 { gen_op_sfne_imm_t0, gen_op_sfne_imm_t1, gen_op_sfne_imm_t2 };

static const generic_gen_op l_sfne_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sfne_t0_t0, gen_op_sfne_t0_t1, gen_op_sfne_t0_t2 },
/* param0 -> t1 */ { gen_op_sfne_t1_t0, gen_op_sfne_t1_t1, gen_op_sfne_t1_t2 },
/* param0 -> t2 */ { gen_op_sfne_t2_t0, gen_op_sfne_t2_t1, gen_op_sfne_t2_t2 } };

void gen_l_sfne(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(!param[0] && !param[1]) {
    gen_op_set_flag(opq, 1);
    return;
  }

  if(!param[0]) {
    if(param_t[1] == T_NONE)
      if(param[1])
        gen_op_set_flag(opq, 1);
      else
        gen_op_clear_flag(opq, 1);
    else
      l_sfne_null_t_table[param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    l_sfne_null_t_table[param_t[0]](opq, 1);
    return;
  }

  if(param_t[1] == T_NONE)
    l_sfne_imm_t_table[param_t[0]](opq, 1, param[1]);
  else
    l_sfne_t_table[param_t[0]][param_t[1]](opq, 1);
}

static const imm_gen_op l_sll_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sll_imm_t0_t0, gen_op_sll_imm_t0_t1, gen_op_sll_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_sll_imm_t1_t0, gen_op_sll_imm_t1_t1, gen_op_sll_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_sll_imm_t2_t0, gen_op_sll_imm_t2_t1, gen_op_sll_imm_t2_t2 } };

static const generic_gen_op l_sll_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_sll_t0_t0_t0, gen_op_sll_t0_t0_t1, gen_op_sll_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_sll_t0_t1_t0, gen_op_sll_t0_t1_t1, gen_op_sll_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_sll_t0_t2_t0, gen_op_sll_t0_t2_t1, gen_op_sll_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_sll_t1_t0_t0, gen_op_sll_t1_t0_t1, gen_op_sll_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_sll_t1_t1_t0, gen_op_sll_t1_t1_t1, gen_op_sll_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_sll_t1_t2_t0, gen_op_sll_t1_t2_t1, gen_op_sll_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_sll_t2_t0_t0, gen_op_sll_t2_t0_t1, gen_op_sll_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_sll_t2_t1_t0, gen_op_sll_t2_t1_t1, gen_op_sll_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_sll_t2_t2_t0, gen_op_sll_t2_t2_t1, gen_op_sll_t2_t2_t2 } } };

void gen_l_sll(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_sll_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_sll_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const imm_gen_op l_sra_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_sra_imm_t0_t0, gen_op_sra_imm_t0_t1, gen_op_sra_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_sra_imm_t1_t0, gen_op_sra_imm_t1_t1, gen_op_sra_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_sra_imm_t2_t0, gen_op_sra_imm_t2_t1, gen_op_sra_imm_t2_t2 } };

static const generic_gen_op l_sra_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_sra_t0_t0_t0, gen_op_sra_t0_t0_t1, gen_op_sra_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_sra_t0_t1_t0, gen_op_sra_t0_t1_t1, gen_op_sra_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_sra_t0_t2_t0, gen_op_sra_t0_t2_t1, gen_op_sra_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_sra_t1_t0_t0, gen_op_sra_t1_t0_t1, gen_op_sra_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_sra_t1_t1_t0, gen_op_sra_t1_t1_t1, gen_op_sra_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_sra_t1_t2_t0, gen_op_sra_t1_t2_t1, gen_op_sra_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_sra_t2_t0_t0, gen_op_sra_t2_t0_t1, gen_op_sra_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_sra_t2_t1_t0, gen_op_sra_t2_t1_t1, gen_op_sra_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_sra_t2_t2_t0, gen_op_sra_t2_t2_t1, gen_op_sra_t2_t2_t2 } } };

void gen_l_sra(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_sra_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_sra_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const imm_gen_op l_srl_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_srl_imm_t0_t0, gen_op_srl_imm_t0_t1, gen_op_srl_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_srl_imm_t1_t0, gen_op_srl_imm_t1_t1, gen_op_srl_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_srl_imm_t2_t0, gen_op_srl_imm_t2_t1, gen_op_srl_imm_t2_t2 } };

static const generic_gen_op l_srl_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_srl_t0_t0_t0, gen_op_srl_t0_t0_t1, gen_op_srl_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_srl_t0_t1_t0, gen_op_srl_t0_t1_t1, gen_op_srl_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_srl_t0_t2_t0, gen_op_srl_t0_t2_t1, gen_op_srl_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_srl_t1_t0_t0, gen_op_srl_t1_t0_t1, gen_op_srl_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_srl_t1_t1_t0, gen_op_srl_t1_t1_t1, gen_op_srl_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_srl_t1_t2_t0, gen_op_srl_t1_t2_t1, gen_op_srl_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_srl_t2_t0_t0, gen_op_srl_t2_t0_t1, gen_op_srl_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_srl_t2_t1_t0, gen_op_srl_t2_t1_t1, gen_op_srl_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_srl_t2_t2_t0, gen_op_srl_t2_t2_t1, gen_op_srl_t2_t2_t2 } } };

void gen_l_srl(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if(!param[1]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_srl_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_srl_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

static const generic_gen_op l_neg_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_neg_t0_t0, gen_op_neg_t0_t1, gen_op_neg_t0_t2 },
/* param0 -> t1 */ { gen_op_neg_t1_t0, gen_op_neg_t1_t1, gen_op_neg_t1_t2 },
/* param0 -> t2 */ { gen_op_neg_t2_t0, gen_op_neg_t2_t1, gen_op_neg_t2_t2 } };

static const generic_gen_op l_sub_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_sub_t0_t0_t0, gen_op_sub_t0_t0_t1, gen_op_sub_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_sub_t0_t1_t0, gen_op_sub_t0_t1_t1, gen_op_sub_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_sub_t0_t2_t0, gen_op_sub_t0_t2_t1, gen_op_sub_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_sub_t1_t0_t0, gen_op_sub_t1_t0_t1, gen_op_sub_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_sub_t1_t1_t0, gen_op_sub_t1_t1_t1, gen_op_sub_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_sub_t1_t2_t0, gen_op_sub_t1_t2_t1, gen_op_sub_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_sub_t2_t0_t0, gen_op_sub_t2_t0_t1, gen_op_sub_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_sub_t2_t1_t0, gen_op_sub_t2_t1_t1, gen_op_sub_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_sub_t2_t2_t0, gen_op_sub_t2_t2_t1, gen_op_sub_t2_t2_t2 } } };

void gen_l_sub(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if((param_t[2] != T_NONE) && (param[1] == param[2])) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[1] && !param[2]) {
    clear_t[param_t[0]](opq, 1);
    return;
  }

  if(!param[1]) {
    if(param_t[2] == T_NONE)
      mov_t_imm[param_t[0]](opq, 1, -param[2]);
    else
      l_neg_t_table[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(!param[2]) {
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  l_sub_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

/* FIXME: This will not work if the l.sys is in a delay slot */
void gen_l_sys(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(do_stats)
    gen_op_analysis(opq, 1, 7, 0x20000000 | param[0]);

  if(!delay_slot)
    gen_op_prep_sys(opq, 1);
  else
    gen_op_prep_sys_delay(opq, 1);

  gen_op_do_sched(opq, 1);
  gen_op_do_jump(opq, 1);
}

/* FIXME: This will not work if the l.trap is in a delay slot */
void gen_l_trap(struct op_queue *opq, int param_t[3], orreg_t param[3],
                int delay_slot)
{
  if(do_stats)
    gen_op_analysis(opq, 1, 8, 0x22000000);

  if(!delay_slot)
    gen_op_prep_trap(opq, 1);
  else
    gen_op_prep_trap_delay(opq, 1);
}

static const imm_gen_op l_xor_imm_t_table[NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */ { gen_op_xor_imm_t0_t0, gen_op_xor_imm_t0_t1, gen_op_xor_imm_t0_t2 },
/* param0 -> t1 */ { gen_op_xor_imm_t1_t0, gen_op_xor_imm_t1_t1, gen_op_xor_imm_t1_t2 },
/* param0 -> t2 */ { gen_op_xor_imm_t2_t0, gen_op_xor_imm_t2_t1, gen_op_xor_imm_t2_t2 } };

static const generic_gen_op l_xor_t_table[NUM_T_REGS][NUM_T_REGS][NUM_T_REGS] = {
/* param0 -> t0 */              {
/* param0 -> t0, param1 -> t0 */ { gen_op_xor_t0_t0_t0, gen_op_xor_t0_t0_t1, gen_op_xor_t0_t0_t2 },
/* param0 -> t0, param1 -> t1 */ { gen_op_xor_t0_t1_t0, gen_op_xor_t0_t1_t1, gen_op_xor_t0_t1_t2 },
/* param0 -> t0, param1 -> t2 */ { gen_op_xor_t0_t2_t0, gen_op_xor_t0_t2_t1, gen_op_xor_t0_t2_t2 } },
/* param0 -> t1 */              {
/* param0 -> t1, param1 -> t0 */ { gen_op_xor_t1_t0_t0, gen_op_xor_t1_t0_t1, gen_op_xor_t1_t0_t2 },
/* param0 -> t1, param1 -> t1 */ { gen_op_xor_t1_t1_t0, gen_op_xor_t1_t1_t1, gen_op_xor_t1_t1_t2 },
/* param0 -> t1, param1 -> t2 */ { gen_op_xor_t1_t2_t0, gen_op_xor_t1_t2_t1, gen_op_xor_t1_t2_t2 } },
/* param0 -> t2 */              {
/* param0 -> t2, param1 -> t0 */ { gen_op_xor_t2_t0_t0, gen_op_xor_t2_t0_t1, gen_op_xor_t2_t0_t2 },
/* param0 -> t2, param1 -> t1 */ { gen_op_xor_t2_t1_t0, gen_op_xor_t2_t1_t1, gen_op_xor_t2_t1_t2 },
/* param0 -> t2, param1 -> t2 */ { gen_op_xor_t2_t2_t0, gen_op_xor_t2_t2_t1, gen_op_xor_t2_t2_t2 } } };

void gen_l_xor(struct op_queue *opq, int param_t[3], orreg_t param[3],
               int delay_slot)
{
  if(!param[0])
    return;

  if((param_t[2] != T_NONE) && (param[1] == param[2])) {
    clear_t[param_t[0]](opq, 1);
    return;
  }
    
  if(!param[2]) {
    if((param_t[2] == T_NONE) && (param[0] == param[1]))
      return;
    move_t_t[param_t[0]][param_t[1]](opq, 1);
    return;
  }

  if(!param[1]) {
    if(param_t[2] == T_NONE) {
      mov_t_imm[param_t[0]](opq, 1, param[2]);
      return;
    }
    move_t_t[param_t[0]][param_t[2]](opq, 1);
    return;
  }

  if(param_t[2] == T_NONE)
    l_xor_imm_t_table[param_t[0]][param_t[1]](opq, 1, param[2]);
  else
    l_xor_t_table[param_t[0]][param_t[1]][param_t[2]](opq, 1);
}

void gen_l_invalid(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  if(!delay_slot)
    gen_op_illegal(opq, 1);
  else
    gen_op_illegal_delay(opq, 1);
}

/*----------------------------------[ Floating point instructions (stubs) ]---*/
void gen_lf_add_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                  int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_div_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                  int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_ftoi_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_itof_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_madd_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_mul_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                  int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_rem_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                  int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sfeq_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sfge_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sfgt_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sfle_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sflt_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sfne_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                   int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

void gen_lf_sub_s(struct op_queue *opq, int param_t[3], orreg_t param[3],
                  int delay_slot)
{
  gen_l_invalid(opq, param_t, param, delay_slot);
}

