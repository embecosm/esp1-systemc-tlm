/* arch.h -- OR1K architecture specific macros
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

#define LINK_REG "r9"
#define LINK_REGNO (9)
#define STACK_REG "r1"
#define STACK_REGNO (1)
#define FRAME_REG "r2"
#define FRAME_REGNO (2)
#define RETURNV_REG "r11"
#define RETURNV_REGNO (11)

/* Basic types for openrisc */
typedef uint32_t oraddr_t; /* Address as addressed by openrisc */
typedef uint32_t uorreg_t; /* An unsigned register of openrisc */
typedef int32_t orreg_t; /* A signed register of openrisc */

#define PRIxADDR "08" PRIx32 /* How to print an openrisc address in hex */
#define PRIxREG "08" PRIx32 /* How to print an openrisc register in hex */
#define PRIdREG PRId32 /* How to print an openrisc register in decimals */

#define ADDR_C(c) UINT32_C(c)
#define REG_C(c) UINT32_C(c)

/* Should args be passed on stack for simprintf 
 *
 * FIXME: do not enable this since it causes problems 
 *        in some cases (an example beeing cbasic test
 *        from orp testbench). the problems is in
 *
 *        or1k/support/simprintf.c
 * 
 *        #if STACK_ARGS
 *                arg = eval_mem32(argaddr,&breakpoint);
 *                argaddr += 4;
 *        #else
 *                sprintf(regstr, "r%u", ++argaddr);
 *                arg = evalsim_reg(atoi(regstr));
 *        #endif
 *
 *        the access to memory should be without any 
 *        checks (ie not like or32 application accessed it)
 * 
 */ 
#define STACK_ARGS 0
