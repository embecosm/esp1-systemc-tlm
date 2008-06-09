/* file.c -- Definition of functions and structures for 
   peripheral to communicate with host through files
   
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

#define _GNU_SOURCE	/* for strndup */

#include <sys/types.h>	/* open() */
#include <sys/stat.h>	/* open() */
#include <fcntl.h>	/* open() */
#if HAVE_MALLOC_H
#include <malloc.h>	/* calloc, free */
#endif
#include <string.h>	/* strndup(), strchr() */
#include <errno.h>	/* errno */
#include <unistd.h>	/* close() */

#include "channel.h"	/* struct channel_ops */
#include "fd.h"		/* struct fd_channel, fd_read(), fd_write() */

struct file_channel
{
	struct fd_channel fds;
	char * namein;
	char * nameout;
};

static void * file_init(const char * args)
{
	struct file_channel * retval;
	char * nameout;

	if(!args)
	{
		errno = EINVAL;
		return NULL;
	}

	retval = (struct file_channel*)calloc(1, sizeof(struct file_channel));

	if(!retval)
	{
		return NULL;
	}

	retval->fds.fdin  = -1;
	retval->fds.fdout = -1;

	nameout = strchr(args, ',');

	if(nameout)
	{
		retval->namein  = strndup(args, nameout - args);
		retval->nameout = strdup(nameout+1);
	}
	else
	{
		retval->nameout = retval->namein  = strdup(args);
	}

	return (void*)retval;
}

static int file_open(void * data)
{
	struct file_channel * files = (struct file_channel *)data;

	if(!files)
	{
		errno = ENODEV;
		return -1;
	}

	if(files->namein == files->nameout)
	{
		/* if we have the same name in and out 
		 * it cannot (logically) be a regular files.
		 * so we wont create one
		 */
		files->fds.fdin = files->fds.fdout = open(files->namein, O_RDWR);

		return files->fds.fdin < 0 ? -1 : 0;
	}


	files->fds.fdin = open(files->namein, O_RDONLY | O_CREAT, 0664);

	if(files->fds.fdin < 0)
		return -1;

	files->fds.fdout = open(files->nameout, O_WRONLY | O_CREAT, 0664);

	if(files->fds.fdout < 0)
	{
		close(files->fds.fdout);
		files->fds.fdout = -1;
		return -1;
	}

	return 0;
}

static void file_close(void * data)
{
	struct file_channel * files = (struct file_channel *)data;

	if(files->fds.fdin != files->fds.fdout)
		close(files->fds.fdin);

	close(files->fds.fdout);

	files->fds.fdin  = -1;
	files->fds.fdout = -1;
}

static void file_free(void * data)
{
	struct file_channel * files = (struct file_channel *)data;

	if(files->namein != files->nameout)
		free(files->namein);

	free(files->nameout);

	free(files);
}

struct channel_ops file_channel_ops =
{
	init:	file_init,
	open:	file_open,
	close:	file_close,
	read:	fd_read,
	write:	fd_write,
	free:	file_free,
};

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
