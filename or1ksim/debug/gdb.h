/* config.h -- Simulator configuration header file
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

#ifndef GDB_H
#define GDB_H

#include <sys/types.h>

#define DEBUG_SLOWDOWN (1)

enum enum_errors  /* modified <chris@asics.ws> CZ 24/05/01 */
{
  /* Codes > 0 are for system errors */

  ERR_NONE = 0,
  ERR_CRC = -1,
  ERR_MEM = -2,
  JTAG_PROXY_INVALID_COMMAND = -3,
  JTAG_PROXY_SERVER_TERMINATED = -4,
  JTAG_PROXY_NO_CONNECTION = -5,
  JTAG_PROXY_PROTOCOL_ERROR = -6,
  JTAG_PROXY_COMMAND_NOT_IMPLEMENTED = -7,
  JTAG_PROXY_INVALID_CHAIN = -8,
  JTAG_PROXY_INVALID_ADDRESS = -9,
  JTAG_PROXY_ACCESS_EXCEPTION = -10, /* Write to ROM */
  JTAG_PROXY_INVALID_LENGTH = -11,
  JTAG_PROXY_OUT_OF_MEMORY = -12,
};

/* This is repeated from gdb tm-or1k.h There needs to be
   a better mechanism for tracking this, but I don't see
   an easy way to share files between modules. */

typedef enum {
  JTAG_COMMAND_READ = 1,
  JTAG_COMMAND_WRITE = 2,
  JTAG_COMMAND_BLOCK_READ = 3,
  JTAG_COMMAND_BLOCK_WRITE = 4,
  JTAG_COMMAND_CHAIN = 5,
} JTAG_proxy_protocol_commands;

/* Each transmit structure must begin with an integer
   which specifies the type of command. Information
   after this is variable. Make sure to have all information
   aligned properly. If we stick with 32 bit integers, it
   should be portable onto every platform. These structures
   will be transmitted across the network in network byte
   order.
*/

typedef struct {
  uint32_t command;
  uint32_t length;
  uint32_t address;
  uint32_t data_H;
  uint32_t data_L;
} JTAGProxyWriteMessage;

typedef struct {
  uint32_t command;
  uint32_t length;
  uint32_t address;
} JTAGProxyReadMessage;

typedef struct {
  uint32_t command;
  uint32_t length;
  uint32_t address;
  int32_t  nRegisters;
  uint32_t data[1];
} JTAGProxyBlockWriteMessage;

typedef struct {
  uint32_t command;
  uint32_t length;
  uint32_t address;
  int32_t  nRegisters;
} JTAGProxyBlockReadMessage;

typedef struct {
  uint32_t command;
  uint32_t length;
  uint32_t chain;
} JTAGProxyChainMessage;

/* The responses are messages specific, however convention
   states the first word should be an error code. Again,
   sticking with 32 bit integers should provide maximum
   portability. */

typedef struct {
  int32_t status;
} JTAGProxyWriteResponse;

typedef struct {
  int32_t status;
  uint32_t data_H;
  uint32_t data_L;
} JTAGProxyReadResponse;
  
typedef struct {
  int32_t status;
} JTAGProxyBlockWriteResponse;

typedef struct {
  int32_t status;
  int32_t nRegisters;
  uint32_t data[1];
  /* uint32_t data[nRegisters-1] still unread */
} JTAGProxyBlockReadResponse;

typedef struct {
  int32_t status;
} JTAGProxyChainResponse;

#endif /* GDB_H */
