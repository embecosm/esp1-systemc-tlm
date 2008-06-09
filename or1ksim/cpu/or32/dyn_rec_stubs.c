/* dyn_rec_stubs.c -- Stubs to allow the recompiler to be run standalone
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


/* Stubs to test the recompiler */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <byteswap.h>
#include <stdlib.h>
#include <inttypes.h>

#include "arch.h"
#include "immu.h"
#include "spr_defs.h"
#include "opcode/or32.h"
#include "abstract.h"
#include "execute.h"
#include "sim-config.h"
#include "sched.h"

#include "i386_regs.h"
#include "dyn_rec.h"

#define PAGE_LEN 8192

int do_stats = 0;

/* NOTE: Directly copied from execute.c */
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

oraddr_t immu_translate(oraddr_t virtaddr)
{
  return virtaddr;
}

oraddr_t peek_into_itlb(oraddr_t virtaddr)
{
  return virtaddr;
}

void do_scheduler()
{
  return;
}

static uint32_t page[PAGE_LEN / 4];

uint32_t eval_insn(oraddr_t addr, int *brkp)
{
  if(addr >= PAGE_LEN) {
    fprintf(stderr, "DR is trying to access memory outside the boundries of a page %08x\n", addr);
    return 0;
  }

  return bswap_32(page[addr / 4]);
}

int main(int argc, char **argv)
{
  FILE *f;
  long len; 
  int i;
  struct dyn_page *dp;
  long off = 0;

  if((argc < 3) || (argc > 4)) {
    fprintf(stderr, "Usage: %s <binary file to recompile> <output code file> [offset into the file]\n",
            argv[0]);
    return 1;
  }

  if(argc == 4)
    off = strtol(argv[3], NULL, 0);

  f = fopen(argv[1], "r");
  if(!f) {
    fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  if(fseek(f, 0, SEEK_END)) {
    fprintf(stderr, "Uanble to seek to the end of the file %s: %s\n", argv[1],
            strerror(errno));
    return 1;
  }

  len = ftell(f);

  if(len == -1) {
    fprintf(stderr, "Unable to determine file length: %s\n", strerror(errno));
    return 1;
  }

  fseek(f, off, SEEK_SET);

  if((len - off) < PAGE_LEN) {
    printf("File is less than 1 page long, padding with zeros.\n");
    fread(page, len, 1, f);
    /* Pad the page with zeros */
    for(i = len; i < PAGE_LEN; i++)
      page[i] = 0;
  } else
    fread(page, PAGE_LEN, 1, f);

  fclose(f);

  build_automata();
  init_dyn_recomp();

  dp = new_dp(0);

  /* Cool, recompile the page */
  fprintf(stderr, "Hold on a sec, I'm recompileing the given page...\n");

  recompile_page(dp);

  fprintf(stderr, "Recompiled page length: %i\n", dp->host_len);
  fprintf(stderr, "Recompiled to: %p\n", dp->host_page);
  fprintf(stderr, "Dumping reced page to disk...\n");

  f = fopen(argv[2], "w");
  fwrite(dp->host_page, dp->host_len, 1, f);
  fclose(f);

/*
  printf("--- Recompiled or disassembly ---\n");
  for(i = 0; i < 2048; i++) {
    extern char *disassembled;
    disassemble_insn(eval_insn(i * 4, NULL));
    if(!eval_insn(i * 4, NULL)) continue;
    printf("%04x: %08x %s\n", i * 4, eval_insn(i * 4, NULL), disassembled);
  }
  printf("--- Recompiled or disassembly end ---\n");
*/

  printf("--- Recompiled offsets ---\n");
  for(i = 0; i < (PAGE_LEN / 4); i++)
    printf("%"PRIxADDR": %x\n", i * 4, dp->locs[i] - dp->host_page);
  printf("--- Recompiled offsets end ---\n");
  destruct_automata();

  return 0;
}

/* Lame linker stubs.  These are only referenced in the recompiled code */
struct cpu_state cpu_state;
struct runtime runtime;
struct scheduler_struct scheduler;
struct config config;
int immu_ex_from_insn;

/* FIXME: eval_insn should become this */
uint32_t eval_insn_direct(oraddr_t memaddr, int through_mmu)
{
  return 0;
}

uint32_t eval_direct32(oraddr_t memaddr, int through_mmu, int through_dc)
{
  return 0;
}

uint32_t eval_mem32(oraddr_t addr, int *breakpoint)
{
  return 0;
}

uint16_t eval_mem16(oraddr_t addr, int *breakpoint)
{
  return 0;
}

uint8_t eval_mem8(oraddr_t addr, int *breakpoint)
{
  return 0;
}

void set_mem32(oraddr_t addr, uint32_t val, int *breakpoint)
{
}

void set_mem16(oraddr_t addr, uint16_t val, int *breakpoint)
{
}

void set_mem8(oraddr_t addr, uint8_t val, int *breakpoint)
{
}

void analysis(struct iqueue_entry *current)
{
}

void mtspr(uint16_t regno, const uorreg_t value)
{
}

unsigned long spr_read_ttcr(void)
{
  return 0;
}

void debug(int level, const char *format,...)
{
}

void simprintf(oraddr_t stackaddr, unsigned long regparam)
{
}

const char *except_name(oraddr_t except)
{
  return NULL;
}

uorreg_t mfspr(const uint16_t regno)
{
  return 0;
}

static struct dev_memarea dummy_area = {
 ops: { delayr: 1 },
};

struct dev_memarea *verify_memoryarea(oraddr_t addr)
{
  return &dummy_area;
}

void sim_done (void)
{
}
