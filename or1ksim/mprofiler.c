/* mprofiler.c -- memory profiling utility
   Copyright (C) 2002 Marko Mlinar, markom@opencores.org

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

/* Command line utility, that displays profiling information, generated
   by or1ksim. (use --mprofile option at command line, when running or1ksim.  */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_MALLOC_H
#include <malloc.h>	/* calloc, free */
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "support/profile.h"
#include "mprofiler.h"
#include "sim-config.h"

struct memory_hash {
  struct memory_hash *next;
  oraddr_t addr;
  unsigned long cnt[3];    /* Various counters */
} *hash[HASH_SIZE];

/* Groups size -- how much addresses should be joined together */
int group_bits = 2;

/* Start address */
oraddr_t start_addr = 0;

/* End address */
oraddr_t end_addr = 0xffffffff;

/* File to read from */
static FILE *fprof = 0;

void mp_help (void)
{
  PRINTF ("mprofiler <-d|-p|-a|-w> [-f filename] [-g group] from to\n");
  PRINTF ("\t-d\t--detail\t\tdetailed output\n");
  PRINTF ("\t-p\t--pretty\t\tpretty output\n");
  PRINTF ("\t-a\t--access\t\toutput accesses only\n");
  PRINTF ("\t-w\t--width\t\t\toutput by width\n");
  PRINTF ("\t-f\t--filename filename\tspecify mprofile file [sim.mprofile]\n");
  PRINTF ("\t-g\t--group bits\t\tgroup 2^bits successive\n");
  PRINTF ("\t\t\t\t\taddresses together [2]\n");
  PRINTF ("\t-h\t--help\t\t\toutput this screen\n");
}

void hash_add (oraddr_t addr, int index)
{
  struct memory_hash *h = hash[HASH_FUNC(addr)];
  while (h && h->addr != addr) h = h->next;
  
  if (!h) {
    h = (struct memory_hash *)malloc (sizeof (struct memory_hash));
    h->next = hash[HASH_FUNC(addr)];
    hash[HASH_FUNC(addr)] = h;
    h->addr = addr;
    h->cnt[0] = h->cnt[1] = h->cnt[2] = 0;
  }
  h->cnt[index]++;
}

unsigned long hash_get (oraddr_t addr, int index)
{
  struct memory_hash *h = hash[HASH_FUNC(addr)];
  while (h && h->addr != addr) h = h->next;
  
  if (!h) return 0;
  return h->cnt[index];
}

void init (void)
{
  int i;
  for (i = 0; i < HASH_SIZE; i++)
    hash[i] = NULL;
}

void read_file (FILE *f, int mode)
{
  struct mprofentry_struct buf[BUF_SIZE];
  int num_read;
  do {
    int i;
    num_read = fread (buf, sizeof (struct mprofentry_struct), BUF_SIZE, f);
    for (i = 0; i < num_read; i++) if (buf[i].addr >= start_addr && buf[i].addr <= end_addr) {
      int index;
      unsigned t = buf[i].type;
      if (t > 64) {
        PRINTF ("!");
        t = 0;
      }
      if (mode == MODE_WIDTH) t >>= 3;
      else t &= 0x7;

      switch (t) {
        case 1: index = 0; break;
        case 2: index = 1; break;
        case 4: index = 2; break;
        default:
          index = 0;
          PRINTF ("!!!!");
          break;
      }
      hash_add (buf[i].addr >> group_bits, index);
    }
  } while (num_read > 0);
}

static inline int nbits (unsigned long a)
{
  int cnt = 0;
  int b = a;
  if (!a) return 0;
  
  while (a) a >>= 1, cnt++;
  if (cnt > 1 && ((b >> (cnt - 2)) & 1))
    cnt = cnt * 2 + 1;
  else
    cnt *= 2;
   
  return cnt - 1;
}

void printout (int mode)
{
  oraddr_t addr = start_addr & ~((1 << group_bits) - 1);
  PRINTF ("start = %"PRIxADDR" (%"PRIxADDR"); end = %"PRIxADDR"; group_bits = %08x\n", start_addr, addr, end_addr, (1 << group_bits) - 1);
  for (; addr <= end_addr; addr += (1 << group_bits)) {
    int i;
    unsigned long a = hash_get (addr >> group_bits, 0);
    unsigned long b = hash_get (addr >> group_bits, 1);
    unsigned long c = hash_get (addr >> group_bits, 2);
    PRINTF ("%"PRIxADDR":", addr);
    switch (mode) {
      case MODE_DETAIL:
        if (a) PRINTF (" %10li R", a);
        else PRINTF ("            R");
        if (b) PRINTF (" %10li W", b);
        else PRINTF ("            W");
        if (c) PRINTF (" %10li F", c);
        else PRINTF ("            F");
        break;
      case MODE_ACCESS:
        PRINTF (" %10li", a + b + c);
        break;
      case MODE_PRETTY:
        PRINTF (" %10li ", a + b + c);
        for (i = 0; i < nbits (a + b + c); i++)
          PRINTF ("#");
#if 0
        for (; i < 64; i++)
          PRINTF (".");
#endif
        break;
      case MODE_WIDTH:
        if (a) PRINTF (" %10li B", a);
        else PRINTF ("            B");
        if (b) PRINTF (" %10li H", b);
        else PRINTF ("            H");
        if (c) PRINTF (" %10li W", c);
        else PRINTF ("            W");
        break;
    }
    PRINTF ("\n");
    if (addr >= addr + (1 << group_bits)) break; /* Overflow? */
  }
}

int main_mprofiler (int argc, char *argv[])
{
  char fmprofname[50] = "sim.mprofile";
  int param = 0;
  int mode = MODE_DETAIL;

  argv++; argc--;
  while (argc > 0) {
    if (!strcmp(argv[0], "-d") || !strcmp(argv[0], "--detail")) {
      mode = MODE_DETAIL;
      argv++; argc--;
    } else if (!strcmp(argv[0], "-p") || !strcmp(argv[0], "--pretty")) {
      mode = MODE_PRETTY;
      argv++; argc--;
    } else if (!strcmp(argv[0], "-a") || !strcmp(argv[0], "--access")) {
      mode = MODE_ACCESS;
      argv++; argc--;
    } else if (!strcmp(argv[0], "-w") || !strcmp(argv[0], "--width")) {
      mode = MODE_WIDTH;
      argv++; argc--;
    } else if (!strcmp(argv[0], "-g") || !strcmp(argv[0], "--group")) {
      argv++; argc--;
      group_bits = strtoul (argv[0], NULL, 0);
      argv++; argc--;
    } else if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help")) {
      mp_help ();
      return 0;
    } else if (!strcmp(argv[0], "-f") || !strcmp(argv[0], "--filename")) {
      argv++; argc--;
      strcpy (&fmprofname[0], argv[0]);
      argv++; argc--;
    } else {
      switch (param) {
        case 0:
          start_addr = strtoul (argv[0], NULL, 0);
          break;
        case 1:
          end_addr = strtoul (argv[0], NULL, 0);
          break;
        default:
          fprintf (stderr, "Invalid number of parameters.\n");
          return -1;
      }
      argv++; argc--; param++;
    }
  }

  fprof = fopen (fmprofname, "rm");

  if (!fprof) {
    fprintf (stderr, "Cannot open profile file: %s\n", fmprofname);
    return 1;
  }
  
  init ();
  read_file (fprof, mode);
  fclose (fprof);
  printout (mode);
  return 0;
}
