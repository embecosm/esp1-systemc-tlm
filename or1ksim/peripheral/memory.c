/* memory.c -- Generic memory model
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

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"

struct mem_config {
  int ce;                         /* Which ce this memory is associated with */
  int mc;                         /* Which mc this memory is connected to */
  oraddr_t baseaddr;              /* Start address of the memory */
  unsigned int size;              /* Memory size */
  char *name;                     /* Memory type string */
  char *log;                      /* Memory log filename */
  int delayr;                     /* Read cycles */
  int delayw;                     /* Write cycles */

  void *mem;                      /* malloced memory for this memory */

  int pattern;                    /* A user specified memory initialization
                                   * pattern */
  int random_seed;                /* Initialize the memory with random values,
                                   * starting with seed */
  enum {
    MT_UNKNOWN,
    MT_PATTERN,
    MT_RANDOM
  } type;
};

uint32_t simmem_read32(oraddr_t addr, void *dat)
{
  return *(uint32_t *)(dat + addr);
}

uint16_t simmem_read16(oraddr_t addr, void *dat)
{
#ifdef WORDS_BIGENDIAN
  return *(uint16_t *)(dat + addr);
#else
  return *(uint16_t *)(dat + (addr ^ 2));
#endif
}

uint8_t simmem_read8(oraddr_t addr, void *dat)
{
#ifdef WORDS_BIGENDIAN
  return *(uint8_t *)(dat + addr);
#else
  return *(uint8_t *)(dat + ((addr & ~ADDR_C(3)) | (3 - (addr & 3))));
#endif
}

void simmem_write32(oraddr_t addr, uint32_t value, void *dat)
{
  *(uint32_t *)(dat + addr) = value;
}

void simmem_write16(oraddr_t addr, uint16_t value, void *dat)
{
#ifdef WORDS_BIGENDIAN
  *(uint16_t *)(dat + addr) = value;
#else
  *(uint16_t *)(dat + (addr ^ 2)) = value;
#endif
}

void simmem_write8(oraddr_t addr, uint8_t value, void *dat)
{
#ifdef WORDS_BIGENDIAN
  *(uint8_t *)(dat + addr) = value;
#else
  *(uint8_t *)(dat + ((addr & ~ADDR_C(3)) | (3 - (addr & 3)))) = value;
#endif
}

uint32_t simmem_read_zero32(oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 32-bit memory read from non-read memory area 0x%"
                     PRIxADDR".\n", addr);
  return 0;
}

uint16_t simmem_read_zero16(oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 16-bit memory read from non-read memory area 0x%"
                     PRIxADDR".\n", addr);
  return 0;
}

uint8_t simmem_read_zero8(oraddr_t addr, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 8-bit memory read from non-read memory area 0x%"
                     PRIxADDR".\n", addr);
  return 0;
}

void simmem_write_null32(oraddr_t addr, uint32_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 32-bit memory write to 0x%"PRIxADDR", non-write "
                     "memory area (value 0x%08"PRIx32").\n", addr, value);
}

void simmem_write_null16(oraddr_t addr, uint16_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 16-bit memory write to 0x%"PRIxADDR", non-write "
                     "memory area (value 0x%08"PRIx32").\n", addr, value);
}

void simmem_write_null8(oraddr_t addr, uint8_t value, void *dat)
{
  if (config.sim.verbose)
    fprintf (stderr, "WARNING: 8-bit memory write to 0x%"PRIxADDR", non-write "
                     "memory area (value 0x%08"PRIx32").\n", addr, value);
}

void mem_reset(void *dat)
{
  struct mem_config *mem = dat;
  int seed;
  int i;
  uint8_t *mem_area = mem->mem;

  /* Initialize memory */
  switch(mem->type) {
  case MT_RANDOM:
    if (mem->random_seed == -1) {
      seed = time(NULL);
      /* Print out the seed just in case we ever need to debug */
      PRINTF("Seeding random generator with value %d\n", seed);
    } else
      seed = mem->random_seed;
    srandom(seed);

    for(i = 0; i < mem->size; i++, mem_area++)
      *mem_area = random() & 0xFF;
    break;
  case MT_PATTERN:
    for(i = 0; i < mem->size; i++, mem_area++)
      *mem_area = mem->pattern;
    break;
  case MT_UNKNOWN:
    break;
  default:
    fprintf(stderr, "Invalid memory configuration type.\n");
    exit(1);
  }
}

/*-------------------------------------------------[ Memory configuration ]---*/
void memory_random_seed(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->random_seed = val.int_val;
}

void memory_pattern(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->pattern = val.int_val;
}

void memory_type(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  if(!strcmp(val.str_val, "unknown"))
    mem->type = MT_UNKNOWN;
  else if(!strcmp (val.str_val, "random"))
    mem->type = MT_RANDOM;
  else if(!strcmp (val.str_val, "pattern"))
    mem->type = MT_PATTERN;
  else if(!strcmp (val.str_val, "zero")) {
    mem->type = MT_PATTERN;
    mem->pattern = 0;
  } else {
    char tmp[200];
    sprintf (tmp, "invalid memory type '%s'.\n", val.str_val);
    CONFIG_ERROR(tmp);
  }
}

void memory_ce(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->ce = val.int_val;
}

void memory_mc(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->mc = val.int_val;
}

void memory_baseaddr(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->baseaddr = val.addr_val;
}

void memory_size(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->size = val.int_val;
}

/* FIXME: Check use */
void memory_name(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->name = strdup(val.str_val);
}

void memory_log(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->log = strdup(val.str_val);
}

void memory_delayr(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->delayr = val.int_val;
}

void memory_delayw(union param_val val, void *dat)
{
  struct mem_config *mem = dat;
  mem->delayw = val.int_val;
}

void *memory_sec_start(void)
{
  struct mem_config *mem = malloc(sizeof(struct mem_config));

  if(!mem) {
    fprintf(stderr, "Memory Peripheral: Run out of memory\n");
    exit(-1);
  }
  mem->size = 0;
  mem->log = NULL;
  mem->name = NULL;
  mem->delayr = 1;
  mem->delayw = 1;
  mem->random_seed = -1;
  mem->ce = -1;
  mem->mc = 0;

  return mem;
}

void memory_sec_end(void *dat)
{
  struct mem_config *mem = dat;
  struct dev_memarea *mema;

  struct mem_ops ops;

  if(!mem->size) {
    free(dat);
    return;
  }

  /* Round up to the next 32-bit boundry */
  if(mem->size & 3) {
    mem->size &= ~3;
    mem->size += 4;
  }

  if(!(mem->mem = malloc(mem->size))) {
    fprintf(stderr, "Unable to allocate memory at %"PRIxADDR", length %i\n",
            mem->baseaddr, mem->size);
    exit(-1);
  }

  if(mem->delayr > 0) {
    ops.readfunc32 = simmem_read32;
    ops.readfunc16 = simmem_read16;
    ops.readfunc8 = simmem_read8;
  } else {
    ops.readfunc32 = simmem_read_zero32;
    ops.readfunc16 = simmem_read_zero16;
    ops.readfunc8 = simmem_read_zero8;
  }

  if(mem->delayw > 0) {
    ops.writefunc32 = simmem_write32;
    ops.writefunc16 = simmem_write16;
    ops.writefunc8 = simmem_write8;
  } else {
    ops.writefunc32 = simmem_write_null32;
    ops.writefunc16 = simmem_write_null16;
    ops.writefunc8 = simmem_write_null8;
  }

  ops.writeprog8 = simmem_write8;
  ops.writeprog32 = simmem_write32;
  ops.writeprog8_dat = mem->mem;
  ops.writeprog32_dat = mem->mem;

  ops.read_dat32 = mem->mem;
  ops.read_dat16 = mem->mem;
  ops.read_dat8 = mem->mem;

  ops.write_dat32 = mem->mem;
  ops.write_dat16 = mem->mem;
  ops.write_dat8 = mem->mem;

  ops.delayr = mem->delayr;
  ops.delayw = mem->delayw;

  ops.log = mem->log;

  mema = reg_mem_area(mem->baseaddr, mem->size, 0, &ops);

  /* Set valid */
  /* FIXME: Should this be done during reset? */
  set_mem_valid(mema, 1);

  if(mem->ce >= 0)
    mc_reg_mem_area(mema, mem->ce, mem->mc);

  reg_sim_reset(mem_reset, dat);
}

void reg_memory_sec(void)
{
  struct config_section *sec = reg_config_sec("memory", memory_sec_start,
                                              memory_sec_end);

  reg_config_param(sec, "random_seed", paramt_int, memory_random_seed);
  reg_config_param(sec, "pattern", paramt_int, memory_pattern);
  reg_config_param(sec, "type", paramt_word, memory_type);
  reg_config_param(sec, "ce", paramt_int, memory_ce);
  reg_config_param(sec, "mc", paramt_int, memory_mc);
  reg_config_param(sec, "baseaddr", paramt_addr, memory_baseaddr);
  reg_config_param(sec, "size", paramt_int, memory_size);
  reg_config_param(sec, "name", paramt_str, memory_name);
  reg_config_param(sec, "log", paramt_str, memory_log);
  reg_config_param(sec, "delayr", paramt_int, memory_delayr);
  reg_config_param(sec, "delayw", paramt_int, memory_delayw);
}

