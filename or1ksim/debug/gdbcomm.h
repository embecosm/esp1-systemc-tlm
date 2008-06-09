/* gdbcomm.h -- Communication routines for gdb
   Copyright (C) 2001 by Marko Mlinar, markom@opencores.org
   Code copied from toplevel.c

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gdb.h"
#include <signal.h>
#include <errno.h>
typedef enum {
  false = 0,
  true = 1,
} Boolean;

void HandleServerSocket(Boolean);
void JTAGRequest(void);
void GDBRequest(void);
void ProtocolClean(int,int32_t);
void BlockJTAG(void);
int GetServerSocket(const char* name,const char* proto,int port);

void gdbcomm_init (void);
