/* abstract.h -- Abstract entities header file
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

#include <stdio.h>

#define DEFAULT_MEMORY_START  0
#define DEFAULT_MEMORY_LEN  0x800000
#define STACK_SIZE  20
#define LABELNAME_LEN 50
#define INSNAME_LEN 15
#define OPERANDNAME_LEN 50

#define MAX_OPERANDS    (5)

#define OP_MEM_ACCESS 0x80000000

/* Cache tag types.  */
#define CT_NONE            0
#define CT_VIRTUAL         1
#define CT_PHYSICAL        2

/* Instruction queue */
struct iqueue_entry {
  int insn_index;  
  uint32_t insn;
  oraddr_t insn_addr;
};

struct mem_ops {
  /* Read functions */
  uint32_t (*readfunc32)(oraddr_t, void *);
  uint16_t (*readfunc16)(oraddr_t, void *);
  uint8_t (*readfunc8)(oraddr_t, void *);

  /* Read functions' data */
  void *read_dat8;
  void *read_dat16;
  void *read_dat32;

  /* Write functions */
  void (*writefunc32)(oraddr_t, uint32_t, void *);
  void (*writefunc16)(oraddr_t, uint16_t, void *);
  void (*writefunc8)(oraddr_t, uint8_t, void *);

  /* Write functions' data */
  void *write_dat8;
  void *write_dat16;
  void *write_dat32;

  /* Program load function.  If you have unwritteable memory but you would like
   * it if a program would be loaded here, make sure to set this.  If this is
   * not set, then writefunc8 will be called to load the program */
  void (*writeprog32)(oraddr_t, uint32_t, void *);
  void (*writeprog8)(oraddr_t, uint8_t, void *);

  void *writeprog32_dat;
  void *writeprog8_dat;

  /* Read/Write delays */
  int delayr;
  int delayw;

  /* Name of log file */
  const char *log;
};

/* Memory regions assigned to devices */
struct dev_memarea {
  struct dev_memarea *next;  
  oraddr_t addr_mask;
  oraddr_t addr_compare;
  uint32_t size;
  oraddr_t size_mask;        /* Address mask, calculated out of size */
  
  int valid;                 /* This bit reflects the memory controler valid bit */
  FILE *log;                 /* log file if this device is to be logged, NULL otherwise */

  struct mem_ops ops;
  struct mem_ops direct_ops;
};

extern void dumpmemory(oraddr_t from, oraddr_t to, int disasm, int nl);
extern uint32_t eval_mem32(oraddr_t memaddr,int*);
extern uint16_t eval_mem16(oraddr_t memaddr,int*);
extern uint8_t eval_mem8(oraddr_t memaddr,int*);
void set_mem32(oraddr_t memaddr, uint32_t value,int*);
extern void set_mem16(oraddr_t memaddr, uint16_t value,int*);
extern void set_mem8(oraddr_t memaddr, uint8_t value,int*);

uint32_t evalsim_mem32(oraddr_t, oraddr_t);
uint16_t evalsim_mem16(oraddr_t, oraddr_t);
uint8_t evalsim_mem8(oraddr_t, oraddr_t);

void setsim_mem32(oraddr_t, oraddr_t, uint32_t);
void setsim_mem16(oraddr_t, oraddr_t, uint16_t);
void setsim_mem8(oraddr_t, oraddr_t, uint8_t);

/* Closes files, etc. */
void done_memory_table (void);

/* Displays current memory configuration */
void memory_table_status (void);

/* Register memory area */
struct dev_memarea *reg_mem_area(oraddr_t addr, uint32_t size, unsigned mc_dev,
                                 struct mem_ops *ops);

/* Adjusts the read and write delays for the memory area pointed to by mem. */
void adjust_rw_delay(struct dev_memarea *mem, int delayr, int delayw);

/* Sets the valid bit (Used only by memory controllers) */
void set_mem_valid(struct dev_memarea *mem, int valid);

/* Check if access is to registered area of memory. */
struct dev_memarea *verify_memoryarea(oraddr_t addr);

/* Outputs time in pretty form to dest string. */
char *generate_time_pretty (char *dest, long time_ps);

/* Returns 32-bit values from mem array. */
uint32_t eval_insn(oraddr_t, int *);

uint32_t eval_insn_direct(oraddr_t memaddr, int through_mmu);

uint8_t eval_direct8(oraddr_t memaddr, int through_mmu, int through_dc);
uint16_t eval_direct16(oraddr_t memaddr, int through_mmu, int through_dc);
uint32_t eval_direct32(oraddr_t addr, int through_mmu, int through_dc);

void set_direct8(oraddr_t, uint8_t, int, int);
void set_direct16(oraddr_t, uint16_t, int, int);
void set_direct32(oraddr_t, uint32_t, int, int);

/* Same as set_direct32, but it also writes to memory that is non-writeable to
 * the rest of the sim.  Used to do program loading. */
void set_program32(oraddr_t memaddr, uint32_t value);
void set_program8(oraddr_t memaddr, uint8_t value);

/* Temporary variable to increase speed.  */
extern struct dev_memarea *cur_area;

/* These are set by mmu if cache inhibit bit is set for current acces.  */
extern int data_ci, insn_ci;

/* Added by MM */
#ifndef LONGEST
#define LONGEST long long
#define ULONGEST unsigned long long
#endif /* ! LONGEST */

/* Returns the page that addr belongs to */
#define IADDR_PAGE(addr) ((addr) & config.immu.page_mask)
#define DADDR_PAGE(addr) ((addr) & config.dmmu.page_mask)

/* History of execution */
#define HISTEXEC_LEN 200
struct hist_exec {
  oraddr_t addr;
  struct hist_exec *prev;
  struct hist_exec *next;
};

extern struct hist_exec *hist_exec_tail;
