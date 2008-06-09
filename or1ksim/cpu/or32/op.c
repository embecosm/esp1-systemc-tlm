/* op.c -- Micro operations for the recompiler
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
#include <stdint.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "spr_defs.h"
#include "opcode/or32.h"
#include "sim-config.h"
#include "except.h"
#include "abstract.h"
#include "execute.h"
#include "sprs.h"
#include "sched.h"

#include "op_support.h"

#include "i386_regs.h"

#include "dyn_rec.h"

/* This must be here since the function in op_i386.h use this variable */
register struct cpu_state *env asm(CPU_STATE_REG);

#include "op_i386.h"

/*
 * WARNING: Before going of and wildly editing everything in this file remember
 * the following about its contents:
 * 1) The `functions' don't EVER return.  In otherwords haveing return state-
 *    ments _anywere_ in this file is likely not to work.  This is because
 *    dyngen just strips away the ret from the end of the function and just uses
 *    the function `body'.  If a ret statement is executed _anyware_ inside the
 *    dynamicly generated code, then it is undefined were we shall jump to.
 * 2) Because of 1), try not to have overly complicated functions.  In too
 *    complicated functions, gcc may decide to generate premature `exits'.  This
 *    is what passing the -fno-reorder-blocks command line switch to gcc helps
 *    with.  This is ofcourse not desired and is rather flaky as we don't (and
 *    can't) control the kind of code that gcc generates: It may work for one
 *    and break for another.  The less branches there are the less likely it is
 *    that a premature return shall occur.
 * 3) If gcc decides that it is going to be a basterd then it will optimise a
 *    very simple condition (if/switch) with a premature exit.  But gcc can't
 *    fuck ME over!  Just stick a FORCE_RET; at the END of the offending
 *    function.
 * 4) All operations must start with `op_'.  dyngen ignores all other functions.
 * 5) Local variables are depriciated: They hinder performance.
 * 6) Function calls are expensive as the stack has to be shifted (twice).
 */

/*#define __or_dynop __attribute__((noreturn))*/
#define __or_dynop

/* Temporaries to hold the (simulated) registers in */
register uint32_t t0 asm(T0_REG);
register uint32_t t1 asm(T1_REG);
register uint32_t t2 asm(T2_REG);

#define OP_PARAM1 ((uorreg_t)(&__op_param1))
#define OP_PARAM2 ((uorreg_t)(&__op_param2))
#define OP_PARAM3 ((uorreg_t)(&__op_param3))

extern uorreg_t __op_param1;
extern uorreg_t __op_param2;
extern uorreg_t __op_param3;

#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)

/* Helper function.  Whenever we escape the recompiled code and there is a
 * potential that an exception may happen this function must be called */
static inline void save_t_temporary(void)
{
  env->t0 = t0;
  env->t1 = t1;
  env->t2 = t2;
}

/* Wrapper around do_scheduler.  This is needed because op_do_sched must be as
 * small as possible. */
void do_sched_wrap(void)
{
  save_t_temporary();
  upd_sim_cycles();
  do_scheduler();
}

/* do_scheduler wrapper for instructions that are in the delay slot */
void do_sched_wrap_delay(void)
{
  save_t_temporary();
  upd_sim_cycles();
  env->ts_current = 1;
  /* The PC gets set to the location of the jump, but do_sched increments that
   * so pull it back here to point to the right location again.  This could be
   * done in op_add_pc/op_set_pc_pc_delay but that would enlarge the recompiled
   * code. */
  //env->pc -= 4;
  do_scheduler();
  env->ts_current = 0;
}

void enter_dyn_code(oraddr_t addr, struct dyn_page *dp)
{
  uint16_t reg;

  addr &= config.immu.pagesize - 1;
  addr >>= 2;

  reg = dp->ts_bound[addr];

  if(reg & 0x1f)
    t0 = cpu_state.reg[reg & 0x1f];

  if((reg >> 5) & 0x1f)
    t1 = cpu_state.reg[(reg >> 5) & 0x1f];

  if((reg >> 10) & 0x1f)
    t2 = cpu_state.reg[(reg >> 10) & 0x1f];

  or_longjmp(dp->locs[addr]);
}

__or_dynop void op_t0_imm(void)
{
  t0 = OP_PARAM1;
}

__or_dynop void op_t1_imm(void)
{
  t1 = OP_PARAM1;
}

__or_dynop void op_t2_imm(void)
{
  t2 = OP_PARAM1;
}

__or_dynop void op_clear_t0(void)
{
  t0 = 0;
}

__or_dynop void op_clear_t1(void)
{
  t1 = 0;
}

__or_dynop void op_clear_t2(void)
{
  t2 = 0;
}

__or_dynop void op_move_t0_t1(void)
{
  t0 = t1;
}

__or_dynop void op_move_t0_t2(void)
{
  t0 = t2;
}

__or_dynop void op_move_t1_t0(void)
{
  t1 = t0;
}

__or_dynop void op_move_t1_t2(void)
{
  t1 = t2;
}

__or_dynop void op_move_t2_t0(void)
{
  t2 = t0;
}

__or_dynop void op_move_t2_t1(void)
{
  t2 = t1;
}

__or_dynop void op_set_pc_pc_delay(void)
{
  env->sprs[SPR_PPC] = get_pc();
  /* pc_delay is pulled back 4 since imediatly after this is run, the scheduler
   * runs which also increments it by 4 */
  set_pc(env->pc_delay - 4);
}

__or_dynop void op_set_pc_delay_imm(void)
{
  env->pc_delay = get_pc() + (orreg_t)OP_PARAM1;
  env->delay_insn = 1;
}

__or_dynop void op_set_pc_delay_pc(void)
{
  env->pc_delay = get_pc();
  env->delay_insn = 1;
}

__or_dynop void op_clear_pc_delay(void)
{
  env->pc_delay = 0;
  env->delay_insn = 1;
}

__or_dynop void op_do_jump(void)
{
  do_jump(get_pc());
}

__or_dynop void op_do_jump_delay(void)
{
  do_jump(env->pc_delay);
}

__or_dynop void op_clear_delay_insn(void)
{
  env->delay_insn = 0;
}

__or_dynop void op_set_delay_insn(void)
{
  env->delay_insn = 1;
}

__or_dynop void op_check_delay_slot(void)
{
  if(!env->delay_insn)
    OP_JUMP(OP_PARAM1);
}

__or_dynop void op_jmp_imm(void)
{
  OP_JUMP(OP_PARAM1);
}

__or_dynop void op_set_flag(void)
{
  env->sprs[SPR_SR] |= SPR_SR_F;
}

__or_dynop void op_clear_flag(void)
{
  env->sprs[SPR_SR] &= ~SPR_SR_F;
}

/* Used for the l.bf instruction.  Therefore if the flag is not set, jump over
 * all the jumping stuff */
__or_dynop void op_check_flag(void)
{
  if(!(env->sprs[SPR_SR] & SPR_SR_F)) {
    HANDLE_SCHED(do_sched_wrap, "no_sched_chk_flg");
    OP_JUMP(OP_PARAM1);
  }
}

/* Used for l.bf if the delay slot instruction is on another page */
__or_dynop void op_check_flag_delay(void)
{
  if(env->sprs[SPR_SR] & SPR_SR_F) {
    env->delay_insn = 1;
    env->pc_delay = get_pc() + (orreg_t)OP_PARAM1;
  }
}

/* Used for the l.bnf instruction.  Therefore if the flag is set, jump over all
 * the jumping stuff */
__or_dynop void op_check_not_flag(void)
{
  if(env->sprs[SPR_SR] & SPR_SR_F) {
    HANDLE_SCHED(do_sched_wrap, "no_sched_chk_not_flg");
    OP_JUMP(OP_PARAM1);
  }
}

/* Used for l.bnf if the delay slot instruction is on another page */
__or_dynop void op_check_not_flag_delay(void)
{
  if(!(env->sprs[SPR_SR] & SPR_SR_F)) {
    env->delay_insn = 1;
    env->pc_delay = get_pc() + (orreg_t)OP_PARAM1;
  }
}

__or_dynop void op_set_ts_current(void)
{
  env->ts_current = 1;
}

__or_dynop void op_add_pc(void)
{
  /* FIXME: Optimise */
  set_pc(get_pc() + OP_PARAM1);
}

__or_dynop void op_nop_exit(void)
{
  upd_sim_cycles();
  save_t_temporary();
  op_support_nop_exit();
  FORCE_RET;
}

__or_dynop void op_nop_reset(void)
{
  upd_sim_cycles();
  op_support_nop_reset();
  do_jump(EXCEPT_RESET);
}

__or_dynop void op_nop_printf(void)
{
  save_t_temporary();
  upd_sim_cycles();
  op_support_nop_printf();
  FORCE_RET;
}

__or_dynop void op_nop_report(void)
{
  save_t_temporary();
  upd_sim_cycles();
  op_support_nop_report();
  FORCE_RET;
}

__or_dynop void op_nop_report_imm(void)
{
  save_t_temporary();
  upd_sim_cycles();
  op_support_nop_report_imm(OP_PARAM1);
}

__or_dynop void op_check_null_except_t0_delay(void)
{
  if(!t0) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc() - 4;
    env->delay_insn = 0;
    do_jump(EXCEPT_ILLEGAL);
  }
}


__or_dynop void op_check_null_except_t0(void)
{
  if(!t0) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc();
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void op_check_null_except_t1_delay(void)
{
  if(!t1) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc() - 4;
    env->delay_insn = 0;
    do_jump(EXCEPT_ILLEGAL);
  }
}


__or_dynop void op_check_null_except_t1(void)
{
  if(!t1) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc();
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void op_check_null_except_t2_delay(void)
{
  if(!t2) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc() - 4;
    env->delay_insn = 0;
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void op_check_null_except_t2(void)
{
  if(!t2) {
    /* Do exception */
    env->sprs[SPR_EEAR_BASE] = get_pc();
    do_jump(EXCEPT_ILLEGAL);
  }
}

__or_dynop void op_analysis(void)
{
  env->iqueue.insn_index = OP_PARAM1;
  env->iqueue.insn = OP_PARAM2;
  env->iqueue.insn_addr = get_pc();
  save_t_temporary();
  op_support_analysis();
  FORCE_RET;
}

#define OP_EXTRA

#define OP /
#define OP_CAST(T) (orreg_t)T
#define OP_NAME div
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP /
#define OP_CAST(T) T
#define OP_NAME divu
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP *
#define OP_CAST(T) T
#define OP_NAME mulu
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP -
#define OP_CAST(T) (orreg_t)T
#define OP_NAME sub
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA

#define OP_HAS_IMM

#define OP_EXTRA + ((env->sprs[SPR_SR] & SPR_SR_CY) >> 10)
#define OP +
#define OP_CAST(T) (orreg_t)T
#define OP_NAME addc
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA
#define OP_EXTRA

#define OP +
#define OP_CAST(T) (orreg_t)T
#define OP_NAME add
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP &
#define OP_CAST(T) T
#define OP_NAME and
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP *
#define OP_CAST(T) (orreg_t)T
#define OP_NAME mul
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP |
#define OP_CAST(T) T
#define OP_NAME or
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP <<
#define OP_CAST(T) T
#define OP_NAME sll
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP >>
#define OP_CAST(T) (orreg_t)T
#define OP_NAME sra
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP >>
#define OP_CAST(T) T
#define OP_NAME srl
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#define OP ^
#define OP_CAST(T) T
#define OP_NAME xor
#include "op_arith_op.h"
#undef OP_NAME
#undef OP_CAST
#undef OP

#undef OP_EXTRA
#undef OP_HAS_IMM

#define EXT_NAME extbs
#define EXT_TYPE int8_t
#define EXT_CAST (orreg_t)
#include "op_extend_op.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME extbz
#define EXT_TYPE uint8_t
#define EXT_CAST (uorreg_t)
#include "op_extend_op.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME exths
#define EXT_TYPE int16_t
#define EXT_CAST (orreg_t)
#include "op_extend_op.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define EXT_NAME exthz
#define EXT_TYPE uint16_t
#define EXT_CAST (uorreg_t)
#include "op_extend_op.h"
#undef EXT_CAST
#undef EXT_TYPE
#undef EXT_NAME

#define COMP ==
#define COMP_NAME sfeq
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP !=
#define COMP_NAME sfne
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >
#define COMP_NAME sfgtu
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >=
#define COMP_NAME sfgeu
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <
#define COMP_NAME sfltu
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <=
#define COMP_NAME sfleu
#define COMP_CAST(t) t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >
#define COMP_NAME sfgts
#define COMP_CAST(t) (orreg_t)t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP >=
#define COMP_NAME sfges
#define COMP_CAST(t) (orreg_t)t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <
#define COMP_NAME sflts
#define COMP_CAST(t) (orreg_t)t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define COMP <=
#define COMP_NAME sfles
#define COMP_CAST(t) (orreg_t)t
#include "op_comp_op.h"
#undef COMP_CAST
#undef COMP_NAME
#undef COMP

#define REG 1
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 2
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 3
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 4
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 5
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 6
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 7
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 8
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 9
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 10
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 11
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 12
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 13
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 14
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 15
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 16
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 17
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 18
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 19
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 20
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 21
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 22
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 23
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 24
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 25
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 26
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 27
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 28
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 29
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 30
#include "op_t_reg_mov_op.h"
#undef REG

#define REG 31
#include "op_t_reg_mov_op.h"
#undef REG

#define DST_T t0
#define SRC_T t0
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t1
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t2
#include "op_ff1_op.h"
#undef SRC_T
#undef DST_T

#define DST_T t1
#define SRC_T t0
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t1
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t2
#include "op_ff1_op.h"
#undef SRC_T
#undef DST_T

#define DST_T t2
#define SRC_T t0
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t1
#include "op_ff1_op.h"
#undef SRC_T

#define SRC_T t2
#include "op_ff1_op.h"
#undef SRC_T
#undef DST_T

#define SPR_T_NAME SPR_T
#define GPR_T_NAME GPR_T

#define SPR_T t0
#define GPR_T t0
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t1
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t2
#include "op_mftspr_op.h"
#undef GPR_T
#undef SPR_T

#define SPR_T t1
#define GPR_T t0
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t1
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t2
#include "op_mftspr_op.h"
#undef GPR_T
#undef SPR_T

#define SPR_T t2
#define GPR_T t0
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t1
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t2
#include "op_mftspr_op.h"
#undef GPR_T
#undef SPR_T

#undef GPR_T_NAME
#undef SPR_T_NAME

#define SPR_T_NAME imm
#define GPR_T_NAME GPR_T

#define SPR_T 0

#define GPR_T t0
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t1
#include "op_mftspr_op.h"
#undef GPR_T

#define GPR_T t2
#include "op_mftspr_op.h"
#undef GPR_T
#undef SPR_T

#undef SPR_T_NAME
#undef GPR_T_NAME

#define ONLY_MTSPR
#define SPR_T_NAME SPR_T
#define GPR_T_NAME clear

#define GPR_T 0

#define SPR_T t0
#include "op_mftspr_op.h"
#undef SPR_T

#define SPR_T t1
#include "op_mftspr_op.h"
#undef SPR_T

#define SPR_T t2
#include "op_mftspr_op.h"
#undef SPR_T

#undef GPR_T

#undef SPR_T_NAME
#undef GPR_T_NAME

#define SPR_T_NAME imm
#define GPR_T_NAME clear
#define GPR_T 0
#define SPR_T 0
#include "op_mftspr_op.h"
#undef SPR_T
#undef GPR_T
#undef GPR_T_NAME
#undef SPR_T_NAME

#undef ONLY_MTSPR

#define OP +=
#define OP_NAME mac
#include "op_mac_op.h"
#undef OP_NAME
#undef OP

#define OP -=
#define OP_NAME msb
#include "op_mac_op.h"
#undef OP_NAME
#undef OP

#define LS_OP_NAME lbz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem8
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lbs
#define LS_OP_CAST (int8_t)
#define LS_OP_FUNC eval_mem8
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lhz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem16
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lhs
#define LS_OP_CAST (int16_t)
#define LS_OP_FUNC eval_mem16
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lwz
#define LS_OP_CAST
#define LS_OP_FUNC eval_mem32
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define LS_OP_NAME lws
#define LS_OP_CAST (int32_t)
#define LS_OP_FUNC eval_mem32
#include "op_lwhb_op.h"
#undef LS_OP_FUNC
#undef LS_OP_CAST
#undef LS_OP_NAME

#define S_OP_NAME sb
#define S_FUNC set_mem8
#include "op_swhb_op.h"
#undef S_FUNC
#undef S_OP_NAME

#define S_OP_NAME sh
#define S_FUNC set_mem16
#include "op_swhb_op.h"
#undef S_FUNC
#undef S_OP_NAME

#define S_OP_NAME sw
#define S_FUNC set_mem32
#include "op_swhb_op.h"
#undef S_FUNC
#undef S_OP_NAME

__or_dynop void op_join_mem_cycles(void)
{
  join_mem_cycles();
}

__or_dynop void op_store_link_addr_gpr(void)
{
  env->reg[LINK_REGNO] = get_pc() + 8;
}

__or_dynop void op_prep_rfe(void)
{
  env->sprs[SPR_SR] = env->sprs[SPR_ESR_BASE] | SPR_SR_FO;
  env->sprs[SPR_PPC] = get_pc();
  env->ts_current = 1;
  set_pc(env->sprs[SPR_EPCR_BASE] - 4);
}

static inline void prep_except(oraddr_t epcr_base)
{
  env->sprs[SPR_EPCR_BASE] = epcr_base;

  env->sprs[SPR_ESR_BASE] = env->sprs[SPR_SR];

  /* Address translation is always disabled when starting exception. */
  env->sprs[SPR_SR] &= ~SPR_SR_DME;
  env->sprs[SPR_SR] &= ~SPR_SR_IME;

  env->sprs[SPR_SR] &= ~SPR_SR_OVE;   /* Disable overflow flag exception. */

  env->sprs[SPR_SR] |= SPR_SR_SM;     /* SUPV mode */
  env->sprs[SPR_SR] &= ~(SPR_SR_IEE | SPR_SR_TEE);    /* Disable interrupts. */
}

__or_dynop void op_set_except_pc(void)
{
  set_pc(OP_PARAM1);
}

/* Before the code in op_{sys,trap}{,_delay} gets run, the scheduler runs.
 * Therefore the pc will point to the instruction after the l.sys or l.trap
 * instruction */
__or_dynop void op_prep_sys_delay(void)
{
  env->delay_insn = 0;
  env->ts_current = 1;
  prep_except(get_pc() - 4);
  set_pc(EXCEPT_SYSCALL - 4);
}

__or_dynop void op_prep_sys(void)
{
  env->ts_current = 1;
  prep_except(get_pc() + 4);
  set_pc(EXCEPT_SYSCALL - 4);
}

__or_dynop void op_prep_trap_delay(void)
{
  env->ts_current = 1;
  env->delay_insn = 0;
  prep_except(get_pc() - 4);
  set_pc(EXCEPT_TRAP - 4);
}

__or_dynop void op_prep_trap(void)
{
  env->ts_current = 1;
  prep_except(get_pc());
  set_pc(EXCEPT_TRAP - 4);
}

/* FIXME: This `instruction' should be split up like the l.trap and l.sys
 * instructions are done */
__or_dynop void op_illegal_delay(void)
{
  env->delay_insn = 0;
  env->ts_current = 1;
  env->sprs[SPR_EEAR_BASE] = get_pc() - 4;
  do_jump(EXCEPT_ILLEGAL - 4);
}

__or_dynop void op_illegal(void)
{
  env->sprs[SPR_EEAR_BASE] = get_pc();
  do_jump(EXCEPT_ILLEGAL);
}

__or_dynop void op_do_sched(void)
{
  HANDLE_SCHED(do_sched_wrap, "no_sched");
}

__or_dynop void op_do_sched_delay(void)
{
  HANDLE_SCHED(do_sched_wrap_delay, "no_sched_delay");
}

__or_dynop void op_macc(void)
{
  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void op_store_insn_ea(void)
{
  env->insn_ea = OP_PARAM1;
}

__or_dynop void op_calc_insn_ea_t0(void)
{
  env->insn_ea = t0 + OP_PARAM1;
}

__or_dynop void op_calc_insn_ea_t1(void)
{
  env->insn_ea = t1 + OP_PARAM1;
}

__or_dynop void op_calc_insn_ea_t2(void)
{
  env->insn_ea = t2 + OP_PARAM1;
}

__or_dynop void op_macrc_t0(void)
{
  /* FIXME: How is this supposed to work?  The architechture manual says that
   * the low 32-bits shall be saved into rD.  I have just copied this code from
   * insnset.c to make testbench/mul pass */
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  t0 = (orreg_t)(temp >> 28);
  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void op_macrc_t1(void)
{
  /* FIXME: How is this supposed to work?  The architechture manual says that
   * the low 32-bits shall be saved into rD.  I have just copied this code from
   * insnset.c to make testbench/mul pass */
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  t1 = (orreg_t)(temp >> 28);

  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void op_macrc_t2(void)
{
  /* FIXME: How is this supposed to work?  The architechture manual says that
   * the low 32-bits shall be saved into rD.  I have just copied this code from
   * insnset.c to make testbench/mul pass */
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  t2 = (orreg_t)(temp >> 28);

  env->sprs[SPR_MACLO] = 0;
  env->sprs[SPR_MACHI] = 0;
}

__or_dynop void op_mac_imm_t0(void)
{
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  temp += (int64_t)t0 * (int64_t)OP_PARAM1;

  env->sprs[SPR_MACLO] = temp & 0xffffffff;
  env->sprs[SPR_MACHI] = temp >> 32;
}

__or_dynop void op_mac_imm_t1(void)
{
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  temp += (int64_t)t1 * (int64_t)OP_PARAM1;

  env->sprs[SPR_MACLO] = temp & 0xffffffff;
  env->sprs[SPR_MACHI] = temp >> 32;
}

__or_dynop void op_mac_imm_t2(void)
{
  int64_t temp = env->sprs[SPR_MACLO] | ((int64_t)env->sprs[SPR_MACHI] << 32);

  temp += (int64_t)t2 * (int64_t)OP_PARAM1;

  env->sprs[SPR_MACLO] = temp & 0xffffffff;
  env->sprs[SPR_MACHI] = temp >> 32;
}

__or_dynop void op_cmov_t0_t0_t1(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t1;
}

__or_dynop void op_cmov_t0_t0_t2(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t2;
}

__or_dynop void op_cmov_t0_t1_t0(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t0;
}

__or_dynop void op_cmov_t0_t1_t2(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t2;
  FORCE_RET;
}

__or_dynop void op_cmov_t0_t2_t0(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t0;
}

__or_dynop void op_cmov_t0_t2_t1(void)
{
  t0 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t1;
  FORCE_RET;
}

__or_dynop void op_cmov_t1_t0_t1(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t1;
}

__or_dynop void op_cmov_t1_t0_t2(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t2;
  FORCE_RET;
}

__or_dynop void op_cmov_t1_t1_t0(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t0;
}

__or_dynop void op_cmov_t1_t1_t2(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t2;
}

__or_dynop void op_cmov_t1_t2_t0(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t0;
  FORCE_RET;
}

__or_dynop void op_cmov_t1_t2_t1(void)
{
  t1 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t1;
}

__or_dynop void op_cmov_t2_t0_t1(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t1;
  FORCE_RET;
}

__or_dynop void op_cmov_t2_t0_t2(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t0 : t2;
}

__or_dynop void op_cmov_t2_t1_t0(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t0;
  FORCE_RET;
}

__or_dynop void op_cmov_t2_t1_t2(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t1 : t2;
}

__or_dynop void op_cmov_t2_t2_t0(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t0;
}

__or_dynop void op_cmov_t2_t2_t1(void)
{
  t2 = env->sprs[SPR_SR] & SPR_SR_F ? t2 : t1;
}

__or_dynop void op_neg_t0_t0(void)
{
  t0 = -t0;
}

__or_dynop void op_neg_t0_t1(void)
{
  t0 = -t1;
}

__or_dynop void op_neg_t0_t2(void)
{
  t0 = -t2;
}

__or_dynop void op_neg_t1_t0(void)
{
  t1 = -t0;
}

__or_dynop void op_neg_t1_t1(void)
{
  t1 = -t1;
}

__or_dynop void op_neg_t1_t2(void)
{
  t1 = -t2;
}

__or_dynop void op_neg_t2_t0(void)
{
  t2 = -t0;
}

__or_dynop void op_neg_t2_t1(void)
{
  t2 = -t1;
}

__or_dynop void op_neg_t2_t2(void)
{
  t2 = -t2;
}

