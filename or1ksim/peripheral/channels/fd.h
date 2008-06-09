/* generic.h -- Declaration of generic functions for peripheral to 
 * communicate with host
   
   Copyright (C) 2002 Richard Prescott <rip@step.polymtl.ca>

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

#ifndef FD_H
#define FD_H

struct fd_channel
{
	int fdin;
	int fdout;
};

int fd_read(void * data, char * buffer, int size);
int fd_write(void * data, const char * buffer, int size);

#endif
