# Makefile -- Makefile for OR32 dependent simulation
# Copyright (C) 1999 Damjan Lampret, lampret@opencores.org
#
# This file is part of OpenRISC 1000 Architectural Simulator.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

noinst_LTLIBRARIES = libarch.la

if DYNAMIC_EXECUTION

# Dynamic execution (--enable-execution=dynamic)

noinst_PROGRAMS       = dyngen \
			test_dyn_rec

test_dyn_rec_SOURCES  = op.c \
			dyn_rec.c \
			op_support.c \
			gen_ops.c \
			gen_ops_gen.c \
			gen_ops_rel.c \
			dyn_rec_stubs.c \
			or32.c
test_dyn_rec_CFLAGS   = $(AM_CFLAGS)

dyngen_SOURCES 	      = dyngen.c \
			dyngen_i386.c \
			dyngen_elf.c
dyngen_CFLAGS         = $(AM_CFLAGS)

libarch_la_SOURCES    = execute.c \
			or32.c \
			op.c \
			dyn_rec.c \
			op_support.c \
			gen_ops.c \
			gen_ops_gen.c \
			gen_ops_rel.c

dyn_rec.lo: gen_ops.h

OP_CFLAGS=-Wall -fomit-frame-pointer -fno-reorder-blocks -O2

op.S: op.c op_t_reg_mov_op.h op_i386.h op_arith_op.h op_comp_op.h op_extend_op.h op_ff1_op.h op_mac_op.h op_mftspr_op.h op_lwhb_op.h op_swhb_op.h
	$(CC) $(INCLUDES) $(OP_CFLAGS) -o $@ -S $<

gen_ops.h gen_ops.c gen_ops_gen.c gen_ops_rel.c: dyngen op.lo

op.lo: op.c op_t_reg_mov_op.h op_i386.h op_arith_op.h op_comp_op.h op_extend_op.h op_ff1_op.h op_mac_op.h op_mftspr_op.h op_lwhb_op.h op_swhb_op.h
	$(CC) $(INCLUDES) $(OP_CFLAGS) -o $@ -c $<

gen_ops.h gen_ops.c gen_ops_gen.c gen_ops_rel.c: dyngen$(EXEEXT) op.lo
	./dyngen$(EXEEXT) op.lo gen_ops.c gen_ops_gen.c gen_ops_rel.c gen_ops.h

else

libarch_la_SOURCES = execute.c or32.c


if GENERATE_NEEDED

# Complex execution (--enable-execution=complex). The default

noinst_PROGRAMS       = generate

generate_SOURCES      = or32.c \
			generate.c
generate_CFLAGS       = $(AM_CFLAGS)

BUILT_SOURCES 	      = execgen.c

execgen.c: generate$(EXEEXT) $(srcdir)/insnset.c
	./generate$(EXEEXT) $(srcdir)/insnset.c execgen.c


else

# Simple execution (--enable-execution=simple)

noinst_PROGRAMS =

endif
endif

# If the simulator was first built without --enable-simple and then with it,
# then also remove these two files

CLEANFILES=execgen.c generate gen_ops.c gen_ops.h gen_ops_rel.c gen_ops_gen.c
