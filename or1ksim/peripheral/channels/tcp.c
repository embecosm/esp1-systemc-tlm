/* tcp.c -- Definition of functions for peripheral to 
 * communicate with host via a tcp socket.
   
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "channel.h"
#include "generic.h"
#include "fd.h"

struct tcp_channel
{
	struct fd_channel fds;
	int socket_fd;				/* Socket to listen to */
	int port_number;			/* TCP port number */
	int connected;				/* If 0, no remote endpoint yet */
	int nonblocking;			/* If 0, read/write will block until 
								   remote client connects */
};

static void * tcp_init(const char * input)
{
	int port_number, fd, flags;
	struct sockaddr_in local_ip;
	struct tcp_channel* channel = (struct tcp_channel*)malloc(sizeof(struct tcp_channel));
	if (!channel)
		return NULL;

	fd = 0;
	channel->nonblocking = 1;
	channel->fds.fdin = -1;
	channel->fds.fdout = -1;
	channel->socket_fd = -1;
	channel->port_number = -1;
		
	port_number = atoi(input);
	if (port_number == 0)
		goto error;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		goto error;

	flags = 1;
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(const char*)&flags,sizeof(int)) < 0)
    {
		perror("Can not set SO_REUSEADDR option on channel socket");
		goto error;
	}

	memset(&local_ip, 0, sizeof(local_ip));
	local_ip.sin_family = AF_INET;
	local_ip.sin_addr.s_addr = htonl(INADDR_ANY);
	local_ip.sin_port = htons(port_number);
	if (bind(fd, (struct sockaddr*)&local_ip, sizeof(local_ip)) < 0) {
		perror("Can't bind local address");
		goto error;
	}

	if (channel->nonblocking) {
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		{
			perror("Can not make channel socket non-blocking");
			goto error;
		}
	}

	if (listen(fd, 1) < 0)
		goto error;

	channel->socket_fd = fd;
	channel->port_number = port_number;
	channel->connected = 0;
	return (void*)channel;
	
error:
	if (fd)
		close(fd);
	free(channel);
	return NULL;
}

static int tcp_open(void * data)
{
	/* Socket is opened lazily, upon first read or write, so do nothing here */
	return 0;
}


static int wait_for_tcp_connect(struct tcp_channel* channel)
{		
	int fd;
	socklen_t sizeof_remote_ip;
	struct sockaddr_in remote_ip;

	sizeof_remote_ip = sizeof(remote_ip);
	fd = accept(channel->socket_fd, (struct sockaddr*)&remote_ip, &sizeof_remote_ip);
	if (fd < 0) {
		if (channel->nonblocking) {
			/* Not an error if there is not yet a remote connection - try again later */
			if (errno == EAGAIN)
				return 0;
		}
		perror("Couldn't accept connection");
		return -1;
	}

	channel->fds.fdin = channel->fds.fdout = fd;
	close(channel->socket_fd);
	channel->socket_fd = -1;
    channel->connected = 1;
	return 1;
}

static int tcp_read(void * data, char * buffer, int size)
{
	struct tcp_channel* channel = data;

	/* Lazily connect to tcp partner on read/write */
	if (!channel->connected) {
		int retval = wait_for_tcp_connect(data);
		if (retval <= 0)
			return retval;
	}
	return fd_read(data, buffer, size);
}

static int tcp_write(void * data, const char * buffer, int size)
{
	struct tcp_channel* channel = data;

	/* Lazily connect to tcp partner on read/write */
	if (!channel->connected) {
		int retval = wait_for_tcp_connect(data);
		if (retval < 0)
			return retval;
	}
	return fd_write(data, buffer, size);
}

struct channel_ops tcp_channel_ops =
{
	init:	tcp_init,
	open:	tcp_open,
	close:	generic_close,
	read:	tcp_read,
	write:	tcp_write,
	free:	generic_free,
};

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

