/* execute.c -- OR1K architecture dependent simulation
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
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

/* Most of the OR1K simulation is done in here.

   When SIMPLE_EXECUTION is defined below a file insnset.c is included!
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "branch_predict.h"
#include "abstract.h"
#include "labels.h"
#include "parse.h"
#include "except.h"
#include "sim-config.h"
#include "debug_unit.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "immu.h"
#include "dmmu.h"
#include "debug.h"
#include "stats.h"
#include "misc.h"

/* Current cpu state */
struct cpu_state cpu_state;

/* Benchmark multi issue execution */
int multissue[20];
int issued_per_cycle = 4;

/* Temporary program counter */
oraddr_t pcnext;

/* Store buffer analysis - stores are accumulated and commited when IO is idle */
static int sbuf_head = 0, sbuf_tail = 0, sbuf_count = 0;
static int sbuf_buf[MAX_SBUF_LEN] = {0};
static int sbuf_prev_cycles = 0;

/* Num cycles waiting for stores to complete */
int sbuf_wait_cyc = 0;

/* Number of total store cycles */
int sbuf_total_cyc = 0;

/* Whether we are doing statistical analysis */
int do_stats = 0;

/* Local data needed for execution.  */
static int next_delay_insn;
static int breakpoint;


/* History of execution */
struct hist_exec *hist_exec_tail = NULL;

/* Implementation specific.
   Get an actual value of a specific register. */

uorreg_t evalsim_reg(unsigned int regno)
{
  if (regno < MAX_GPRS) {
    return cpu_state.reg[regno];
  } else {
    PRINTF("\nABORT: read out of registers\n");
    sim_done();
    return 0;
  }
}

/* Implementation specific.
   Set a specific register with value. */

void setsim_reg(unsigned int regno, uorreg_t value)
{
  if (regno == 0)               /* gpr0 is always zero */
    value = 0;
  
  if (regno < MAX_GPRS) {
    cpu_state.reg[regno] = value;
  } else {
    PRINTF("\nABORT: write out of registers\n");
    sim_done();
  }
}

/* Implementation specific.
   Set a specific register with value. */

inline static void set_reg(int regno, uorreg_t value)
{
#if 0
  if (strcmp(regstr, FRAME_REG) == 0) {
    PRINTF("FP (%s) modified by insn at %x. ", FRAME_REG, cpu_state.pc);
    PRINTF("Old:%.8lx  New:%.8lx\n", eval_reg(regno), value);
  }
  
  if (strcmp(regstr, STACK_REG) == 0) {
    PRINTF("SP (%s) modified by insn at %x. ", STACK_REG, cpu_state.pc);
    PRINTF("Old:%.8lx  New:%.8lx\n", eval_reg(regno), value);
  }
#endif

  if (regno < MAX_GPRS) {
    cpu_state.reg[regno] = value;
#if RAW_RANGE_STATS
    raw_stats.reg[regno] = runtime.sim.cycles;
#endif /* RAW_RANGE */
  } else {
    PRINTF("\nABORT: write out of registers\n");
    sim_done();
  }
}

/* Implementation specific.
   Evaluates source operand opd. */

#if !(DYNAMIC_EXECUTION)
static
#endif
uorreg_t eval_operand_val(uint32_t insn, struct insn_op_struct *opd)
{
  unsigned long operand = 0;
  unsigned long sbit;
  unsigned int nbits = 0;

  while(1) {
    operand |= ((insn >> (opd->type & OPTYPE_SHR)) & ((1 << opd->data) - 1)) << nbits;
    nbits += opd->data;

    if(opd->type & OPTYPE_OP)
      break;
    opd++;
  }

  if(opd->type & OPTYPE_SIG) {
    sbit = (opd->type & OPTYPE_SBIT) >> OPTYPE_SBIT_SHR;
    if(operand & (1 << sbit)) operand |= ~REG_C(0) << sbit;
  }

  return operand;
}

/* Does source operand depend on computation of dstoperand? Return
   non-zero if yes.

 Cycle t                 Cycle t+1
dst: irrelevant         src: immediate                  always 0
dst: reg1 direct        src: reg2 direct                0 if reg1 != reg2
dst: reg1 disp          src: reg2 direct                always 0
dst: reg1 direct        src: reg2 disp                  0 if reg1 != reg2
dst: reg1 disp          src: reg2 disp                  always 1 (store must
                                                        finish before load)
dst: flag               src: flag                       always 1
*/

static int check_depend(prev, next)
     struct iqueue_entry *prev;
     struct iqueue_entry *next;
{
  /* Find destination type. */
  unsigned long type = 0;
  int prev_dis, next_dis;
  orreg_t prev_reg_val = 0;
  struct insn_op_struct *opd;

  if (or32_opcodes[prev->insn_index].flags & OR32_W_FLAG
      && or32_opcodes[next->insn_index].flags & OR32_R_FLAG)
    return 1;

  opd = op_start[prev->insn_index];
  prev_dis = 0;

  while (1) {
    if (opd->type & OPTYPE_DIS)
      prev_dis = 1;

    if (opd->type & OPTYPE_DST) {
      type = opd->type;
      if (prev_dis)
        type |= OPTYPE_DIS;
      /* Destination is always a register */
      prev_reg_val = eval_operand_val (prev->insn, opd);
      break;
    }
    if (opd->type & OPTYPE_LAST)
      return 0; /* Doesn't have a destination operand */
    if (opd->type & OPTYPE_OP)
      prev_dis = 0;
    opd++;
  }

  /* We search all source operands - if we find confict => return 1 */
  opd = op_start[next->insn_index];
  next_dis = 0;

  while (1) {
    if (opd->type & OPTYPE_DIS)
      next_dis = 1;
    /* This instruction sequence also depends on order of execution:
     * l.lw r1, k(r1)
     * l.sw k(r1), r4
     * Here r1 is a destination in l.sw */
    /* FIXME: This situation is not handeld here when r1 == r2:
     * l.sw k(r1), r4
     * l.lw r3, k(r2)
     */
    if (!(opd->type & OPTYPE_DST) || (next_dis && (opd->type & OPTYPE_DST))) {
      if (opd->type & OPTYPE_REG)
        if (eval_operand_val (next->insn, opd) == prev_reg_val)
          return 1;
    }
    if (opd->type & OPTYPE_LAST)
      break;
    opd++;
  }

  return 0;
}

/* Sets a new SPR_SR_OV value, based on next register value */

#if SET_OV_FLAG
#define set_ov_flag(value) \
  if((value) & 0x80000000) \
    cpu_state.sprs[SPR_SR] |= SPR_SR_OV; \
  else \
    cpu_state.sprs[SPR_SR] &= ~SPR_SR_OV
#else
#define set_ov_flag(value)
#endif

/* Modified by CZ 26/05/01 for new mode execution */
/* Fetch returns nonzero if instruction should NOT be executed.  */
static inline int fetch(void)
{
  static int break_just_hit = 0;

  if (CHECK_BREAKPOINTS) {
    /* MM: Check for breakpoint.  This has to be done in fetch cycle,
       because of peripheria.  
       MM1709: if we cannot access the memory entry, we could not set the
       breakpoint earlier, so just check the breakpoint list.  */
    if (has_breakpoint (peek_into_itlb (cpu_state.pc)) && !break_just_hit) {
      break_just_hit = 1;
      return 1; /* Breakpoint set. */
    }
    break_just_hit = 0;
  }

  breakpoint = 0;
  cpu_state.iqueue.insn_addr = cpu_state.pc;
  cpu_state.iqueue.insn = eval_insn (cpu_state.pc, &breakpoint);

  /* Fetch instruction. */
  if (!except_pending)
    runtime.cpu.instructions++;

  /* update_pc will be called after execution */

  return 0;
}

/* This code actually updates the PC value.  */
static inline void update_pc (void)
{
  cpu_state.delay_insn = next_delay_insn;
  cpu_state.sprs[SPR_PPC] = cpu_state.pc; /* Store value for later */
  cpu_state.pc = pcnext;
  pcnext = cpu_state.delay_insn ? cpu_state.pc_delay : pcnext + 4;
}

#if SIMPLE_EXECUTION
static inline 
#endif
void analysis (struct iqueue_entry *current)
{
  if (config.cpu.dependstats) {
    /* Dynamic, dependency stats. */
    adddstats(cpu_state.icomplet.insn_index, current->insn_index, 1,
              check_depend(&cpu_state.icomplet, current));

    /* Dynamic, functional units stats. */
    addfstats(or32_opcodes[cpu_state.icomplet.insn_index].func_unit,
              or32_opcodes[current->insn_index].func_unit, 1,
              check_depend(&cpu_state.icomplet, current));

    /* Dynamic, single stats. */
    addsstats(current->insn_index, 1);
  }

  if (config.cpu.superscalar) {
    if ((or32_opcodes[current->insn_index].func_unit == it_branch) ||
        (or32_opcodes[current->insn_index].func_unit == it_jump))
      runtime.sim.storecycles += 0;
    
    if (or32_opcodes[current->insn_index].func_unit == it_store)
      runtime.sim.storecycles += 1;
    
    if (or32_opcodes[current->insn_index].func_unit == it_load)
      runtime.sim.loadcycles += 1;
#if 0        
    if ((cpu_state.icomplet.func_unit == it_load) &&
        check_depend(&cpu_state.icomplet, current))
      runtime.sim.loadcycles++;
#endif
    
    /* Pseudo multiple issue benchmark */
    if ((multissue[or32_opcodes[current->insn_index].func_unit] < 1) ||
        (check_depend(&cpu_state.icomplet, current)) || (issued_per_cycle < 1)) {
      int i;
      for (i = 0; i < 20; i++)
        multissue[i] = 2;
      issued_per_cycle = 2;
      runtime.cpu.supercycles++;
      if (check_depend(&cpu_state.icomplet, current))
        runtime.cpu.hazardwait++;
      multissue[it_unknown] = 2;
      multissue[it_shift] = 2;
      multissue[it_compare] = 1;
      multissue[it_branch] = 1;
      multissue[it_jump] = 1;
      multissue[it_extend] = 2;
      multissue[it_nop] = 2;
      multissue[it_move] = 2;
      multissue[it_movimm] = 2;
      multissue[it_arith] = 2;
      multissue[it_store] = 2;
      multissue[it_load] = 2;
    }
    multissue[or32_opcodes[current->insn_index].func_unit]--;
    issued_per_cycle--;
  }
        
  if (config.cpu.dependstats)
    /* Instruction waits in completition buffer until retired. */
    memcpy (&cpu_state.icomplet, current, sizeof (struct iqueue_entry));

  if (config.sim.history) {
    /* History of execution */
    hist_exec_tail = hist_exec_tail->next;
    hist_exec_tail->addr = cpu_state.icomplet.insn_addr;
  }

  if (config.sim.exe_log) dump_exe_log();
}

/* Store buffer analysis - stores are accumulated and commited when IO is idle */
static inline void sbuf_store (int cyc) {
  int delta = runtime.sim.cycles - sbuf_prev_cycles;
  sbuf_total_cyc += cyc;
  sbuf_prev_cycles = runtime.sim.cycles;
  
  //PRINTF (">STORE %i,%i,%i,%i,%i\n", delta, sbuf_count, sbuf_tail, sbuf_head, sbuf_buf[sbuf_tail], sbuf_buf[sbuf_head]);
  //PRINTF ("|%i,%i\n", sbuf_total_cyc, sbuf_wait_cyc);
  /* Take stores from buffer, that occured meanwhile */
  while (sbuf_count && delta >= sbuf_buf[sbuf_tail]) {
    delta -= sbuf_buf[sbuf_tail];
    sbuf_tail = (sbuf_tail + 1) % MAX_SBUF_LEN;
    sbuf_count--;
  }
  if (sbuf_count)
    sbuf_buf[sbuf_tail] -= delta;

  /* Store buffer is full, take one out */
  if (sbuf_count >= config.cpu.sbuf_len) {
    sbuf_wait_cyc += sbuf_buf[sbuf_tail];
    runtime.sim.mem_cycles += sbuf_buf[sbuf_tail];
    sbuf_prev_cycles += sbuf_buf[sbuf_tail];
    sbuf_tail = (sbuf_tail + 1) % MAX_SBUF_LEN;
    sbuf_count--;
  }
  /* Put newest store in the buffer */
  sbuf_buf[sbuf_head] = cyc;
  sbuf_head = (sbuf_head + 1) % MAX_SBUF_LEN;
  sbuf_count++;
  //PRINTF ("|STORE %i,%i,%i,%i,%i\n", delta, sbuf_count, sbuf_tail, sbuf_head, sbuf_buf[sbuf_tail], sbuf_buf[sbuf_head]);
}

/* Store buffer analysis - previous stores should commit, before any load */
static inline void sbuf_load () {
  int delta = runtime.sim.cycles - sbuf_prev_cycles;
  sbuf_prev_cycles = runtime.sim.cycles;
  
  //PRINTF (">LOAD  %i,%i,%i,%i,%i\n", delta, sbuf_count, sbuf_tail, sbuf_head, sbuf_buf[sbuf_tail], sbuf_buf[sbuf_head]);
  //PRINTF ("|%i,%i\n", sbuf_total_cyc, sbuf_wait_cyc);
  /* Take stores from buffer, that occured meanwhile */
  while (sbuf_count && delta >= sbuf_buf[sbuf_tail]) {
    delta -= sbuf_buf[sbuf_tail];
    sbuf_tail = (sbuf_tail + 1) % MAX_SBUF_LEN;
    sbuf_count--;
  }
  if (sbuf_count)
    sbuf_buf[sbuf_tail] -= delta;

  /* Wait for all stores to complete */
  while (sbuf_count > 0) {
    sbuf_wait_cyc += sbuf_buf[sbuf_tail];
    runtime.sim.mem_cycles += sbuf_buf[sbuf_tail];
    sbuf_prev_cycles += sbuf_buf[sbuf_tail];
    sbuf_tail = (sbuf_tail + 1) % MAX_SBUF_LEN;
    sbuf_count--;
  }
  //PRINTF ("|LOAD  %i,%i,%i,%i,%i\n", delta, sbuf_count, sbuf_tail, sbuf_head, sbuf_buf[sbuf_tail], sbuf_buf[sbuf_head]);
}

/* Outputs dissasembled instruction */
void dump_exe_log (void)
{
  oraddr_t insn_addr = cpu_state.iqueue.insn_addr;
  unsigned int i, j;
  uorreg_t operand;

  if (insn_addr == 0xffffffff) return;
  if ((config.sim.exe_log_start <= runtime.cpu.instructions) &&
      ((config.sim.exe_log_end <= 0) ||
       (runtime.cpu.instructions <= config.sim.exe_log_end))) {
    if (config.sim.exe_log_marker &&
        !(runtime.cpu.instructions % config.sim.exe_log_marker)) {
      fprintf (runtime.sim.fexe_log, "--------------------- %8lli instruction ---------------------\n", runtime.cpu.instructions);
    }
    switch (config.sim.exe_log_type) {
    case EXE_LOG_HARDWARE:
      fprintf (runtime.sim.fexe_log, "\nEXECUTED(%11llu): %"PRIxADDR":  ",
               runtime.cpu.instructions, insn_addr);
      fprintf (runtime.sim.fexe_log, "%.2x%.2x",
               eval_direct8(insn_addr, 0, 0),
               eval_direct8(insn_addr + 1, 0, 0));
      fprintf (runtime.sim.fexe_log, "%.2x%.2x",
               eval_direct8(insn_addr + 2, 0, 0),
               eval_direct8(insn_addr + 3, 0 ,0));
      for(i = 0; i < MAX_GPRS; i++) {
        if (i % 4 == 0)
          fprintf(runtime.sim.fexe_log, "\n");
        fprintf (runtime.sim.fexe_log, "GPR%2u: %"PRIxREG"  ", i,
                 cpu_state.reg[i]);
      }
      fprintf (runtime.sim.fexe_log, "\n");
      fprintf (runtime.sim.fexe_log, "SR   : %.8"PRIx32"  ",
               cpu_state.sprs[SPR_SR]); 
      fprintf (runtime.sim.fexe_log, "EPCR0: %"PRIxADDR"  ",
               cpu_state.sprs[SPR_EPCR_BASE]);
      fprintf (runtime.sim.fexe_log, "EEAR0: %"PRIxADDR"  ",
               cpu_state.sprs[SPR_EEAR_BASE]);
      fprintf (runtime.sim.fexe_log, "ESR0 : %.8"PRIx32"\n",
               cpu_state.sprs[SPR_ESR_BASE]);
      break;
    case EXE_LOG_SIMPLE:
    case EXE_LOG_SOFTWARE:
      {
        extern char *disassembled;
        disassemble_index (cpu_state.iqueue.insn, cpu_state.iqueue.insn_index);
        {
          struct label_entry *entry;
          entry = get_label(insn_addr);
          if (entry)
            fprintf (runtime.sim.fexe_log, "%s:\n", entry->name);
        }

        if (config.sim.exe_log_type == EXE_LOG_SOFTWARE) {
          struct insn_op_struct *opd = op_start[cpu_state.iqueue.insn_index];
          
          j = 0;
          while (1) {
            operand = eval_operand_val (cpu_state.iqueue.insn, opd);
            while (!(opd->type & OPTYPE_OP))
              opd++;
            if (opd->type & OPTYPE_DIS) {
              fprintf (runtime.sim.fexe_log, "EA =%"PRIxADDR" PA =%"PRIxADDR" ",
                       cpu_state.insn_ea, peek_into_dtlb(cpu_state.insn_ea,0,0));
              opd++; /* Skip of register operand */
              j++;
            } else if ((opd->type & OPTYPE_REG) && operand) {
              fprintf (runtime.sim.fexe_log, "r%-2i=%"PRIxREG" ",
                       (int)operand, evalsim_reg (operand));
            } else
              fprintf (runtime.sim.fexe_log, "             ");
            j++;
            if(opd->type & OPTYPE_LAST)
              break;
            opd++;
          }
          if(or32_opcodes[cpu_state.iqueue.insn_index].flags & OR32_R_FLAG) {
            fprintf (runtime.sim.fexe_log, "SR =%08x ",
                     cpu_state.sprs[SPR_SR]);
            j++;
          }
          while(j < 3) {
            fprintf (runtime.sim.fexe_log, "             ");
            j++;
          }
        }
        fprintf (runtime.sim.fexe_log, "%"PRIxADDR" ", insn_addr);
        fprintf (runtime.sim.fexe_log, "%s\n", disassembled);
      }
    }
  }
}

/* Dump registers - 'r' or 't' command */
void dumpreg()
{
  int i;
  oraddr_t physical_pc;

  if ((physical_pc = peek_into_itlb(cpu_state.iqueue.insn_addr))) {
    /* 
     * PRINTF("\t\t\tEA: %08x <--> PA: %08x\n", cpu_state.iqueue.insn_addr, physical_pc);
     */
    dumpmemory(physical_pc, physical_pc + 4, 1, 0);
  }
  else {
    PRINTF("INTERNAL SIMULATOR ERROR:\n");
    PRINTF("no translation for currently executed instruction\n");
  }
  
  // generate_time_pretty (temp, runtime.sim.cycles * config.sim.clkcycle_ps);
  PRINTF(" (executed) [cycle %lld, #%lld]\n", runtime.sim.cycles,
         runtime.cpu.instructions);
  if (config.cpu.superscalar)
    PRINTF ("Superscalar CYCLES: %u", runtime.cpu.supercycles);
  if (config.cpu.hazards)
    PRINTF ("  HAZARDWAIT: %u\n", runtime.cpu.hazardwait);
  else
    if (config.cpu.superscalar)
      PRINTF ("\n");

  if ((physical_pc = peek_into_itlb(cpu_state.pc))) {
    /*
     * PRINTF("\t\t\tEA: %08x <--> PA: %08x\n", cpu_state.pc, physical_pc);
     */
    dumpmemory(physical_pc, physical_pc + 4, 1, 0);
  }
  else
    PRINTF("%"PRIxADDR": : xxxxxxxx  ITLB miss follows", cpu_state.pc);
  
  PRINTF(" (next insn) %s", (cpu_state.delay_insn?"(delay insn)":""));
  for(i = 0; i < MAX_GPRS; i++) {
    if (i % 4 == 0)
      PRINTF("\n");
    PRINTF("GPR%.2u: %"PRIxREG"  ", i, evalsim_reg(i));
  }
  PRINTF("flag: %u\n", cpu_state.sprs[SPR_SR] & SPR_SR_F ? 1 : 0);
}

/* Generated/built in decoding/executing function */
static inline void decode_execute (struct iqueue_entry *current);

/* Wrapper around real decode_execute function -- some statistics here only */
static inline void decode_execute_wrapper (struct iqueue_entry *current)
{
  breakpoint = 0;

#ifndef HAS_EXECUTION
#error HAS_EXECUTION has to be defined in order to execute programs.
#endif
 
  /* FIXME: Most of this file is not needed with DYNAMIC_EXECUTION */
#if !(DYNAMIC_EXECUTION)
  decode_execute (current);
#endif
  
#if SET_OV_FLAG
  /* Check for range exception */
  if((cpu_state.sprs[SPR_SR] & SPR_SR_OVE) &&
     (cpu_state.sprs[SPR_SR] & SPR_SR_OV))
    except_handle (EXCEPT_RANGE, cpu_state.sprs[SPR_EEAR_BASE]);
#endif

  if(breakpoint)
    except_handle(EXCEPT_TRAP, cpu_state.sprs[SPR_EEAR_BASE]);
}

/* Reset the CPU */
void cpu_reset(void)
{
  int i;
  struct hist_exec *hist_exec_head = NULL;
  struct hist_exec *hist_exec_new;

  runtime.sim.cycles = 0;
  runtime.sim.loadcycles = 0;
  runtime.sim.storecycles = 0;
  runtime.cpu.instructions = 0;
  runtime.cpu.supercycles = 0;
  runtime.cpu.hazardwait = 0;
  for (i = 0; i < MAX_GPRS; i++)
    set_reg (i, 0);
  memset(&cpu_state.iqueue, 0, sizeof(cpu_state.iqueue));
  memset(&cpu_state.icomplet, 0, sizeof(cpu_state.icomplet));
  
  sbuf_head = 0;
  sbuf_tail = 0;
  sbuf_count = 0;
  sbuf_prev_cycles = 0;

  /* Initialise execution history circular buffer */
  for (i = 0; i < HISTEXEC_LEN; i++) {
    hist_exec_new = malloc(sizeof(struct hist_exec));
    if(!hist_exec_new) {
      fprintf(stderr, "Out-of-memory\n");
      exit(1);
    }
    if(!hist_exec_head)
      hist_exec_head = hist_exec_new;
    else
      hist_exec_tail->next = hist_exec_new;

    hist_exec_new->prev = hist_exec_tail;
    hist_exec_tail = hist_exec_new;
  }
  /* Make hist_exec_tail->next point to hist_exec_head */
  hist_exec_tail->next = hist_exec_head;
  hist_exec_head->prev = hist_exec_tail;

  /* Cpu configuration */
  cpu_state.sprs[SPR_UPR] = config.cpu.upr;
  cpu_state.sprs[SPR_VR] = config.cpu.rev & SPR_VR_REV;
  cpu_state.sprs[SPR_VR] |= config.cpu.ver << 16;
  cpu_state.sprs[SPR_SR] = config.cpu.sr;

  pcnext = 0x0; /* MM1409: All programs should start at reset vector entry!  */
  if (config.sim.verbose) PRINTF ("Starting at 0x%"PRIxADDR"\n", pcnext);
  cpu_state.pc = pcnext;
  pcnext += 4;
  debug(1, "reset ...\n");

#if DYNAMIC_EXECUTION
  cpu_state.ts_current = 1;
#endif
  
  /* MM1409: All programs should set their stack pointer!  */
  except_handle(EXCEPT_RESET, 0);
  update_pc();
  except_pending = 0;
}

/* Simulates one CPU clock cycle */
inline int cpu_clock ()
{
  except_pending = 0;
  next_delay_insn = 0;
  if(fetch()) {
    PRINTF ("Breakpoint hit.\n");
    return 1;
  }

  if(except_pending) {
    update_pc();
    except_pending = 0;
    return 0;
  }

  if(breakpoint) {
    except_handle(EXCEPT_TRAP, cpu_state.sprs[SPR_EEAR_BASE]);
    update_pc();
    except_pending = 0;
    return 0;
  }

  decode_execute_wrapper (&cpu_state.iqueue);
  update_pc();
  return 0;
}

/* If decoding cannot be found, call this function */
#if SIMPLE_EXECUTION
void l_invalid (struct iqueue_entry *current) {
#else
void l_invalid () {
#endif
  except_handle(EXCEPT_ILLEGAL, cpu_state.iqueue.insn_addr);
}

#if COMPLEX_EXECUTION

/* Include decode_execute function */ 
#include "execgen.c"

#elif SIMPLE_EXECUTION


#define INSTRUCTION(name) void name (struct iqueue_entry *current)

/* Implementation specific.
   Get an actual value of a specific register. */

static uorreg_t eval_reg(unsigned int regno)
{
  if (regno < MAX_GPRS) {
#if RAW_RANGE_STATS
      int delta = (runtime.sim.cycles - raw_stats.reg[regno]);
      if ((unsigned long)delta < (unsigned long)MAX_RAW_RANGE)
        raw_stats.range[delta]++;
#endif /* RAW_RANGE */
    return cpu_state.reg[regno];
  } else {
    PRINTF("\nABORT: read out of registers\n");
    sim_done();
    return 0;
  }
}

/* Implementation specific.
   Evaluates source operand op_no. */

static uorreg_t eval_operand (int op_no, unsigned long insn_index, uint32_t insn)
{
  struct insn_op_struct *opd = op_start[insn_index];
  uorreg_t ret;

  while (op_no) {
    if(opd->type & OPTYPE_LAST) {
      fprintf (stderr, "Instruction requested more operands than it has\n");
      exit (1);
    }
    if((opd->type & OPTYPE_OP) && !(opd->type & OPTYPE_DIS))
      op_no--;
    opd++;
  }

  if (opd->type & OPTYPE_DIS) {
    ret = eval_operand_val (insn, opd);
    while (!(opd->type & OPTYPE_OP))
      opd++;
    opd++;
    ret += eval_reg (eval_operand_val (insn, opd));
    cpu_state.insn_ea = ret;
    return ret;
  }
  if (opd->type & OPTYPE_REG)
    return eval_reg (eval_operand_val (insn, opd));

  return eval_operand_val (insn, opd);
}

/* Implementation specific.
   Set destination operand (reister direct) with value. */
   
inline static void set_operand(int op_no, orreg_t value,
                               unsigned long insn_index, uint32_t insn)
{
  struct insn_op_struct *opd = op_start[insn_index];

  while (op_no) {
    if(opd->type & OPTYPE_LAST) {
      fprintf (stderr, "Instruction requested more operands than it has\n");
      exit (1);
    }
    if((opd->type & OPTYPE_OP) && !(opd->type & OPTYPE_DIS))
      op_no--;
    opd++;
  }

  if (!(opd->type & OPTYPE_REG)) {
    fprintf (stderr, "Trying to set a non-register operand\n");
    exit (1);
  }
  set_reg (eval_operand_val (insn, opd), value);
}

/* Simple and rather slow decoding function based on built automata. */ 
static inline void decode_execute (struct iqueue_entry *current)
{
  int insn_index;
  
  current->insn_index = insn_index = insn_decode(current->insn);

  if (insn_index < 0)
    l_invalid(current);
  else {
    or32_opcodes[insn_index].exec(current);
  }

  if (do_stats) analysis(&cpu_state.iqueue);
}

#define SET_PARAM0(val) set_operand(0, val, current->insn_index, current->insn)

#define PARAM0 eval_operand(0, current->insn_index, current->insn)
#define PARAM1 eval_operand(1, current->insn_index, current->insn)
#define PARAM2 eval_operand(2, current->insn_index, current->insn)

#include "insnset.c"

#elif defined(DYNAMIC_EXECUTION)

#else
# error "One of SIMPLE_EXECUTION/COMPLEX_EXECUTION must be defined"
#endif
