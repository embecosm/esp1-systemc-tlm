/* stats.h -- Header file for stats.c
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

#define DSTATS_LEN	3000
#define SSTATS_LEN	300
#define FSTATS_LEN	200
#define RAW_RANGE	1000

/* Used by safe division - increment divisor by one if it is zero */
#define SD(X) (X != 0 ? X : 1)

struct dstats_entry {
	int  insn1;
	int  insn2;
	int  cnt_dynamic;
	int  depend;
};

struct sstats_entry {
	int  insn;
	int  cnt_dynamic;
};	/* single stats */

struct fstats_entry {
	enum insn_type insn1;
	enum insn_type insn2;
	int  cnt_dynamic;
	int  depend;
};	/* functional units stats */

struct bpbstat {
	int hit;
	int miss;
	int correct;
	int incorrect;
};

struct bticstat {
	int hit;
	int miss;
};

struct mstats_entry {
	int byteadd;
        int bf[2][2];  /* [taken][fwd/bwd] */
        int bnf[2][2]; /* [taken][fwd/bwd] */
	struct bpbstat bpb;
	struct bticstat btic;
};	/* misc units stats */

struct cachestats_entry {
	int readhit;
	int readmiss;
	int writehit;
	int writemiss;
};	/* cache stats */

struct immustats_entry {
	int fetch_tlbhit;
	int fetch_tlbmiss;
	int fetch_pagefaults;
};	/* IMMU stats */

struct dmmustats_entry {
	int loads_tlbhit;
	int loads_tlbmiss;
	int loads_pagefaults;
	int stores_tlbhit;
	int stores_tlbmiss;
	int stores_pagefaults;
};	/* DMMU stats */

struct raw_stats {
	int reg[64];
	int range[RAW_RANGE];
};	/* RAW hazard stats */

/* Renamed mstats to or1k_mstats because Mac OS X has a lib function called mstats */
extern struct mstats_entry or1k_mstats;
extern struct sstats_entry sstats[SSTATS_LEN];
extern struct dstats_entry dstats[DSTATS_LEN];
extern struct fstats_entry fstats[FSTATS_LEN];
extern struct cachestats_entry ic_stats;
extern struct cachestats_entry dc_stats;
extern struct immustats_entry immu_stats;
extern struct dmmustats_entry dmmu_stats;
extern struct raw_stats raw_stats;

extern void addsstats(int item, int cnt_dynamic);
extern void adddstats(int item1, int item2, int cnt_dynamic, int depend);
extern void addfstats(enum insn_type item1, enum insn_type item2, int cnt_dynamic, int depend);
extern void initstats();
extern void printstats();
