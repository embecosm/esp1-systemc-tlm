/* debug_unit.h -- Simulation of Or1k debug unit
   Copyright (C) 2001 Chris Ziomkowski, chris@asics.ws

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

typedef enum {
  JTAG_CHAIN_GLOBAL = 0,
  JTAG_CHAIN_DEBUG_UNIT = 1,
  JTAG_CHAIN_TEST = 2,
  JTAG_CHAIN_TRACE = 3,
  JTAG_CHAIN_DEVELOPMENT = 4,
  JTAG_CHAIN_WISHBONE = 5,
  JTAG_CHAIN_BLOCK1 = 6,
  JTAG_CHAIN_BLOCK2 = 7,
  JTAG_CHAIN_OPTIONAL0 = 8,
  JTAG_CHAIN_OPTIONAL1 = 9,
  JTAG_CHAIN_OPTIONAL2 = 10,
  JTAG_CHAIN_OPTIONAL3 = 11,
  JTAG_CHAIN_OPTIONAL4 = 12,
  JTAG_CHAIN_OPTIONAL5 = 13,
  JTAG_CHAIN_OPTIONAL6 = 14,
  JTAG_CHAIN_OPTIONAL7 = 15,
} DebugScanChainIDs;


typedef struct {
  unsigned int DCR_hit;
  unsigned int watchpoint;
} DebugUnit;

typedef enum {
  DebugInstructionFetch = 1,
  DebugLoadAddress = 2,
  DebugStoreAddress = 3,
  DebugLoadData = 4,
  DebugStoreData = 5,
} DebugUnitAction;


/* Debug registers and their bits */
typedef struct {
  unsigned long moder;
  unsigned long tsel;
  unsigned long qsel;
  unsigned long ssel;
  unsigned long riscop;
  unsigned long recwp[11];
  unsigned long recbp;
} DevelopmentInterface;

typedef enum {
  DEVELOPINT_MODER = 0,
  DEVELOPINT_TSEL = 1,
  DEVELOPINT_QSEL = 2,
  DEVELOPINT_SSEL = 3,
  DEVELOPINT_RISCOP = 4,
  DEVELOPINT_RECWP0 = 16,
  DEVELOPINT_RECWP1 = 17,
  DEVELOPINT_RECWP2 = 18,
  DEVELOPINT_RECWP3 = 19,
  DEVELOPINT_RECWP4 = 20,
  DEVELOPINT_RECWP5 = 21,
  DEVELOPINT_RECWP6 = 22,
  DEVELOPINT_RECWP7 = 23,
  DEVELOPINT_RECWP8 = 24,
  DEVELOPINT_RECWP9 = 25,
  DEVELOPINT_RECWP10 = 26,
  DEVELOPINT_RECBP0 = 27,
} DevelopmentInterfaceAddressSpace;

#define RISCOP_STALL  0x00000001
#define RISCOP_RESET  0x00000002

/* Resets the debug unit */
void du_reset(void);

/* do cycle-related initialisation (watchpoints etc.) */
void du_clock(void);

/* set cpu_stalled flag */
void set_stall_state (int state);

/* Whether debug unit should ignore exception */
int debug_ignore_exception (unsigned long except);

/* Gets development interface register */
int get_devint_reg(unsigned int addr, uint32_t *data);

/* Sets development interface register */
int set_devint_reg(unsigned int addr, uint32_t data);

/* Reads from bus address */
int debug_get_mem(oraddr_t address, uorreg_t *data);

/* Writes to bus address */
int debug_set_mem(oraddr_t address, uorreg_t data);

int DebugGetRegister(oraddr_t address, uorreg_t *data);

int DebugSetRegister(oraddr_t address, uorreg_t data);

int DebugSetChain(int chain);

#ifdef DEBUGMOD_OFF
#define CheckDebugUnit(x,y) 0
#else
int CheckDebugUnit(DebugUnitAction, unsigned long);
#endif
