/* vapi.h - Verification API Interface
   Copyright (C) 2001, Marko Mlinar, markom@opencores.org

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

#ifndef _VAPI_H_
#define _VAPI_H_

/* Maximum value for VAPI device id */
#define VAPI_MAX_DEVID 0xFFFF  
  
/* Inits the VAPI, according to sim-config */
int vapi_init ();

/* Closes the VAPI */
void vapi_done ();

/* Installs a vapi handler for one VAPI id */
void vapi_install_handler (unsigned long id, void (*read_func) (unsigned long, unsigned long, void *), void *dat);

/* Installs a vapi handler for multiple VAPI id */
void vapi_install_multi_handler (unsigned long base_id, unsigned long num_ids, void (*read_func) (unsigned long, unsigned long, void *), void *dat);

/* Checks for incoming packets */
void vapi_check ();

/* Returns number of unconnected handles.  */
int vapi_num_unconnected (int printout);

/* Sends a packet to specified test */
void vapi_send (unsigned long id, unsigned long data);

#define VAPI_DEVICE_ID (0xff)

/* Types of commands that can be written to the VAPI log file */
typedef enum {
	VAPI_COMMAND_REQUEST = 0, /* Data coming from outside world to device */
	VAPI_COMMAND_SEND = 1, /* Device writing data to the outside world */
	VAPI_COMMAND_END = 2 /* End of log for device */
} VAPI_COMMAND;

/* Writes a line directly to the log file (used by JTAG proxy) */
void vapi_write_log_file(VAPI_COMMAND command, unsigned long device_id, unsigned long data);

#endif /* _VAPI_H_ */
