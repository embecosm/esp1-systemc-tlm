/* dyn_rec.h -- Recompiler specific definitions
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


#ifndef DYN_REC_H
#define DYN_REC_H

/* Each dynamically recompiled page has one of these */
struct dyn_page {
  oraddr_t or_page;
  void *host_page;
  unsigned int host_len;
  int dirty; /* Is recompiled page invalid? */
  int delayr; /* delayr of memory backing this page */
  uint16_t ts_during[2048]; /* What registers the temporaries back (during the
                             * instruction) */
  uint16_t ts_bound[2049]; /* What registers the temporaries back (on the
                            * begining boundry of the instruction) */
  void **locs; /* Openrisc locations in the recompiled code */
};

void recompile_page(struct dyn_page *dyn);
struct dyn_page *new_dp(oraddr_t page);
void add_to_opq(struct op_queue *opq, int end, int op);
void add_to_op_params(struct op_queue *opq, int end, unsigned long param);
void *enough_host_page(struct dyn_page *dp, void *cur, unsigned int *len,
                       unsigned int amount);
void init_dyn_recomp(void);
void jump_dyn_code(oraddr_t addr);
void run_sched_out_of_line(int add_normal);
void recheck_immu(int got_en_dis);
void enter_dyn_code(oraddr_t addr, struct dyn_page *dp);
void dyn_checkwrite(oraddr_t addr);

extern void *rec_stack_base;

#define IMMU_GOT_ENABLED 1
#define IMMU_GOT_DISABLED 2

#endif
