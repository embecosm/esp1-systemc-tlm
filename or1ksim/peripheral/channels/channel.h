/* channel.h -- Definition of types and structures for 
   peripheral to communicate with host.  Addapted from UML.
   
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

#ifndef CHANNEL_H
#define CHANNEL_H

struct channel_ops
{
	void *(*init)(const char *);
	int (*open)(void *);
	void (*close)(void *);
	int (*read)(void *, char *, int);
	int (*write)(void *, const char *, int);
	void (*free)(void *);
	int (*isok)(void*);
	char* (*status)(void*);
};

struct channel
{
	const struct channel_ops * ops;
	void * data;
};


struct channel * channel_init(const char * descriptor);
/* read operation in non-blocking */
int channel_open(struct channel * channel);
int channel_read(struct channel * channel, char * buffer, int size);
int channel_write(struct channel * channel, const char * buffer, int size);
void channel_free(struct channel * channel);
int channel_ok(struct channel * channel);
char * channel_status(struct channel * channel);

void channel_close(struct channel * channel);

int register_channel(const char * name, const struct channel_ops * ops);
/* TODO: int unregister_channel(const char * name); */

/* for those who wants to automatically register at startup */
#define REGISTER_CHANNEL(NAME,OPS) \
  static int NAME ## _dummy_register = register_channel(#NAME, OPS)

#endif

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
