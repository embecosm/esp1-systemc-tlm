/* generic.c -- Definition of generic functions for peripheral to 
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_MALLOC_H
#include <malloc.h>	/* calloc, free */
#endif

#include <errno.h>	/* errno */


int generic_open(void * data)
{
	if(data)
	{
		return 0;
	}
	errno = ENODEV;
	return -1;
}

void generic_close(void * data)
{
	return;
}

void generic_free(void * data)
{
	if(data)
	{
		free(data);
	}
}


/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
