/* labels.h -- Abstract entities header file handling labels
   Copyright (C) 2001 Marko Mlinar, markom@opencores.org

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

#ifndef _LABELS_H_
#define _LABELS_H_

#define LABELS_HASH_SIZE 119

/* Structure for holding one label per particular memory location */
struct label_entry {
  char *name;
  oraddr_t addr;
  struct label_entry *next;
};

struct breakpoint_entry {
  oraddr_t addr;
  struct breakpoint_entry *next;
};

/* Label handling */
void init_labels();
void add_label (oraddr_t addr, char *name);
struct label_entry *get_label (oraddr_t addr);
struct label_entry *find_label (char *name);

/* Searches mem array for a particular label and returns label's address.
   If label does not exist, returns 0. */
oraddr_t eval_label (char *name);

/* Breakpoint handling */
void breakpoints_init ();
void add_breakpoint (oraddr_t addr);
void remove_breakpoint (oraddr_t addr);
void print_breakpoints ();
int has_breakpoint (oraddr_t addr);
void init_breakpoints ();

extern struct breakpoint_entry *breakpoints;
#define CHECK_BREAKPOINTS (breakpoints)

#endif /* _LABELS_H_ */
