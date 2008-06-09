/* simprintf.c -- Simulator printf implementation
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
   
/* Debugger LIBC functions. Working, but VERY, VERY ugly written.
I wrote following code when basic simulator started to work and I was
desperate to use some PRINTFs in my debugged code. And it was
also used to get some output from Dhrystone MIPS benchmark. */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"
#include "sim-config.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(simprintf);

/* Length of PRINTF format string */
#define FMTLEN 2000

char fmtstr[FMTLEN];

char *simgetstr(oraddr_t stackaddr, unsigned long regparam)
{
  oraddr_t fmtaddr;
  int i;

  fmtaddr = regparam;
  
  i = 0;
  while (eval_direct8(fmtaddr,1,0) != '\0') {
    fmtstr[i++] = eval_direct8(fmtaddr,1,0);
    fmtaddr++;
    if (i == FMTLEN - 1)
      break;
  }
  fmtstr[i] = '\0';
  
  return fmtstr;
}

void simprintf(oraddr_t stackaddr, unsigned long regparam)
{
  FILE *f;

  simgetstr(stackaddr, regparam);
  
  TRACE("simprintf: stackaddr: 0x%"PRIxADDR"\n", stackaddr);
  if ((f = fopen(config.sim.fstdout, "a+"))) {
    uint32_t arg;
    oraddr_t argaddr;
    char *fmtstrend;
    char *fmtstrpart = fmtstr;
    int tee_exe_log;
    
#if STACK_ARGS
    argaddr = stackaddr;
#else
    argaddr = 3;
#endif
    tee_exe_log = (config.sim.exe_log && (config.sim.exe_log_type == EXE_LOG_SOFTWARE || config.sim.exe_log_type == EXE_LOG_SIMPLE)
       && config.sim.exe_log_start <= runtime.cpu.instructions && (config.sim.exe_log_end <= 0 || runtime.cpu.instructions <= config.sim.exe_log_end));
       
    if (tee_exe_log) fprintf (runtime.sim.fexe_log, "SIMPRINTF: ");
    TRACE("simprintf: %s\n", fmtstrpart);
    while(strlen(fmtstrpart)) {
      TRACE("simprintf(): 1");
      if ((fmtstrend = strstr(fmtstrpart + 1, "%")))
        *fmtstrend = '\0';
      TRACE(" 2");
      if (strstr(fmtstrpart, "%")) {
        char *tmp;
        int string = 0;
        TRACE(" 3");
#if STACK_ARGS
        arg = eval_direct32(argaddr,1,0);
        argaddr += 4;
#else
	{
	  /* JPB. I can't see how the original code ever worked. It does trash
	     the file pointer by overwriting the end of regstr. In any case
	     why create a string, only to turn it back into an integer! */

	  /* orig:  char regstr[5]; */
	  /* orig:  */
          /* orig:  sprintf(regstr, "r%"PRIxADDR, ++argaddr); */
          /* orig:  arg = evalsim_reg(atoi(regstr)); */

	  arg = evalsim_reg( ++argaddr );
        }
#endif
        TRACE(" 4: fmtstrpart=%p fmtstrpart=%s arg=0x%08"PRIx32"\n",
              fmtstrpart, fmtstrpart, arg);
        tmp = fmtstrpart;
        if (*tmp == '%') {
          tmp++;
          while (*tmp == '-' || (*tmp >= '0' && *tmp <= '9')) tmp++;
          if (*tmp == 's') string = 1;
        }
        if (string) {
          int len = 0;
          char *str;
          for(; eval_direct8(arg++,1,0); len++);
          len++;  /* for null char */
          arg -= len;
          str = (char *)malloc(len);
          len = 0;
          for(; eval_direct8(arg,1,0); len++)
            *(str+len) = eval_direct8(arg++,1,0);
          *(str+len) = eval_direct8(arg,1,0); /* null ch */
          TRACE("4a: len=%d str=%s\n", len, str);
          TRACE("4b:");
          fprintf(f, fmtstrpart, str);
          if (tee_exe_log) fprintf(runtime.sim.fexe_log, fmtstrpart, str);
          free(str);
        } else {
          fprintf(f, fmtstrpart, arg);
          if (tee_exe_log) fprintf(runtime.sim.fexe_log, fmtstrpart, arg);
        }
      } else {
        TRACE(" 5");
        fprintf(f, fmtstrpart);
        if (tee_exe_log) fprintf(runtime.sim.fexe_log, fmtstrpart);
        TRACE(fmtstrpart);
      }
      if (!fmtstrend)
        break;
      TRACE(" 6");
      fmtstrpart = fmtstrend;
      *fmtstrpart = '%';
      TRACE(" 7");
    }
    
    TRACE(" 8\n");
    if (fclose(f))
      perror(strerror(errno));
  }
  else
    perror(strerror(errno));
  
}
