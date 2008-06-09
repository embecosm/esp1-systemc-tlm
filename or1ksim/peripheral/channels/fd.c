/* fd.c -- Definition of functions and structures for 
   peripheral to communicate with host through file descriptors
   
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

#include <sys/time.h>	/* struct timeval */
#include <sys/types.h>	/* fd_set */
#include <stdio.h>	/* perror */
#include <stdlib.h>	/* atoi */
#include <unistd.h>	/* read, write, select */
#if HAVE_MALLOC_H
#include <malloc.h>	/* calloc, free */
#endif
#include <string.h>	/* strchr */
#include <errno.h>	/* errno */

#include "channel.h"
#include "generic.h"
#include "fd.h"

static void * fd_init(const char * args)
{
	struct fd_channel * retval;

	retval = (struct fd_channel*)calloc(1, sizeof(struct fd_channel));

	if(!retval)
	{
		return NULL;
	}


	retval->fdin  = atoi(args);	/* so 0 if garbage */
					/* TODO: strtoul */

	args = strchr(args, ',');

	if(args)
	{
		retval->fdout  = atoi(args+1);
	}
	else
	{
		retval->fdout  = retval->fdin;
	}

	return (void*)retval;
}

int fd_read(void * data, char * buffer, int size)
{
	struct fd_channel * fds = (struct fd_channel *)data;
	struct timeval timeout = { 0, 0 };
	fd_set rfds;
	int retval;

	if(!fds)
	{
		errno = ENODEV;
		return -1;
	}

	FD_ZERO(&rfds);
	FD_SET(fds->fdin, &rfds);

	retval = select(fds->fdin+1, &rfds, NULL, NULL, &timeout);

	if(retval <= 0)
		return retval;

	return read(fds->fdin, buffer, size);
}

int fd_write(void * data, const char * buffer, int size)
{
	struct fd_channel * fds = (struct fd_channel *)data;
	if(fds)
	{
		return write(fds->fdout, buffer, size);
	}
	errno = ENODEV;
	return -1;
}

static int fd_isok(void * data)
{
	struct fd_channel * fds = (struct fd_channel *)data;
	if(fds)
	{
		return fds->fdout != -1 && fds->fdin != -1;
	}
	return 0;
}

static int fd_status_fd(int fd, char * str, int size)
{
	if(fd == -1)
		return snprintf(str, size, "closed");

	return snprintf(str, size, "opened(fd=%d)", fd);
}

char * fd_status(void * data)
{
	static char retval[256];
	int index = 0;

	struct fd_channel * fds = (struct fd_channel *)data;
	if(fds)
	{
		index += snprintf(retval + index, sizeof(retval) - index, "in ");
		index += fd_status_fd(fds->fdin, retval + index, sizeof(retval) - index);

		index += snprintf(retval + index, sizeof(retval) - index, "out ");
		index += fd_status_fd(fds->fdout, retval + index, sizeof(retval) - index);
	}
	else
	{
		snprintf(retval, sizeof(retval), "(null)");
	}
	return retval;
}

struct channel_ops fd_channel_ops =
{
	init:	fd_init,
	open:	generic_open,
	close:	generic_close,
	read:	fd_read,
	write:	fd_write,
	free:	generic_free,
	isok:	fd_isok,
	status:	fd_status,
};

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
