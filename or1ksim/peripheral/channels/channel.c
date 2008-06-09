/* channel.c -- Definition of types and structures for 
   peripherals to communicate with host.  Adapted from UML.
   
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
#include "config.h"
#endif

#define _GNU_SOURCE	/* for strndup */

#include <stdio.h>	/* perror */
#include <stdlib.h>	/* exit */

#if HAVE_MALLOC_H
#include <malloc.h>	/* calloc, free */
#endif

#include <string.h>	/* strndup, strcmp, strlen, strchr */
#include <errno.h>	/* errno */

#include "port.h"

#include "channel.h"

struct channel_factory
{
	const char * name;
	const struct channel_ops * ops;
	struct channel_factory * next;
};

extern struct channel_ops fd_channel_ops, file_channel_ops,
	xterm_channel_ops, tcp_channel_ops, tty_channel_ops;

static struct channel_factory preloaded[] =
{
	{ "fd",     &fd_channel_ops,     &preloaded[1] },
	{ "file",   &file_channel_ops,   &preloaded[2] },
	{ "xterm",  &xterm_channel_ops,  &preloaded[3] },
	{ "tcp",    &tcp_channel_ops,    &preloaded[4] },
	{ "tty",    &tty_channel_ops,    NULL          }
};

static struct channel_factory * head = &preloaded[0];

static struct channel_factory * find_channel_factory(const char * name);

struct channel * channel_init(const char * descriptor)
{
	struct channel * retval;
	struct channel_factory * current;
	char * args, * name;
	int count;

	if(!descriptor)
	{
		return NULL;
	}

	retval = (struct channel*)calloc(1, sizeof(struct channel));

	if(!retval)
	{
		perror(descriptor);
		exit(1);
	}

	args = strchr(descriptor, ':');

	if(args)
	{
		count = args - descriptor;
		args++;
	}
	else
	{
		count = strlen(descriptor);
	}

	name = (char*)strndup(descriptor, count);

	if(!name)
	{
		perror(name);
		exit(1);
	}

	current = find_channel_factory(name);
	
	if(!current)
	{
		errno = ENODEV;
		perror(descriptor);
		exit(1);
	}

	retval->ops = current->ops;

	free(name);

	if(!retval->ops)
	{
		errno = ENODEV;
		perror(descriptor);
		exit(1);
	}

	if(retval->ops->init)
	{
		retval->data = (retval->ops->init)(args);

		if(!retval->data)
		{
			perror(descriptor);
			exit(1);
		}
	}

	return retval;
}

int channel_open(struct channel * channel)
{
	if(channel && channel->ops && channel->ops->open)
	{
		return (channel->ops->open)(channel->data);
	}
	errno = ENOSYS;
	return -1;
}

int channel_read(struct channel * channel, char * buffer, int size)
{
	if(channel && channel->ops && channel->ops->read)
	{
		return (channel->ops->read)(channel->data, buffer, size);
	}
	errno = ENOSYS;
	return -1;
}

int channel_write(struct channel * channel, const char * buffer, int size)
{
	if(channel && channel->ops && channel->ops->write)
	{
		return (channel->ops->write)(channel->data, buffer, size);
	}
	errno = ENOSYS;
	return -1;
}

void channel_close(struct channel * channel)
{
	if(channel && channel->ops && channel->ops->close)
	{
		(channel->ops->close)(channel->data);
	}
}

void channel_free(struct channel * channel)
{
	if(channel && channel->ops && channel->ops->free)
	{
		(channel->ops->free)(channel->data);
		free(channel);
	}
}


int channel_ok(struct channel * channel)
{
	if(channel && channel->ops)
	{
		if(channel->ops->isok)
			return (channel->ops->isok)(channel->data);
		else
			return 1;
	}
	return 0;
}

char * channel_status(struct channel * channel)
{
	if(channel && channel->ops && channel->ops->status)
	{
		return (channel->ops->status)(channel->data);
	}
	return "";
}



static struct channel_factory * find_channel_factory(const char * name)
{
	struct channel_factory * current = head;

	current = head;
	while(current && strcmp(current->name, name))
	{
		current = current->next;
	}

	return current;
}

int register_channel(const char * name, const struct channel_ops * ops)
{
	struct channel_factory * new;


	if(find_channel_factory(name))
	{
		errno = EEXIST;
		perror(name);
		exit(1);
	}

	new = (struct channel_factory *)calloc(1, sizeof(struct channel_factory));

	if(!new)
	{
		perror(name);
		exit(1);
	}

	new->name = name;
	new->ops  = ops;
	new->next = head;
	head = new;

	return (int)new;	/* dummy */
}

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

