/* tty.c -- Definition of functions for peripheral to 
 * communicate with host via a tty.
   
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
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include "channel.h"
#include "generic.h"
#include "fd.h"

// Default parameters if not specified in config file
#define DEFAULT_BAUD B19200
#define DEFAULT_TTY_DEVICE "/dev/ttyS0"

struct tty_channel
{
    struct fd_channel fds;
};

static struct {
	char* name;
	int value;
} baud_table[] = {
	{"50", B50},
	{"2400",   B2400},
	{"4800",   B4800},
	{"9600",   B9600},
	{"19200",  B19200},
	{"38400",  B38400},
	{"115200", B115200},
	{"230400", B230400},
	{0,        0}
};

// Convert baud rate string to termio baud rate constant
int
parse_baud(char* baud_string)
{
	int i;
	for (i = 0; baud_table[i].name; i++) {
		if (!strcmp(baud_table[i].name, baud_string))
			return baud_table[i].value;
	}

	fprintf(stderr, "Error: unknown baud rate: %s\n", baud_string);
	fprintf(stderr, "       Known baud rates: ");

	for (i = 0; baud_table[i].name; i++) {
		fprintf(stderr, "%s%s", baud_table[i].name, baud_table[i+1].name ? ", " : "\n");
	}
	return B0;
}

static void * tty_init(const char * input)
{
	int fd = 0, baud;
	char *param_name, *param_value, *device;
    struct termios options;
    struct tty_channel* channel;

	channel = (struct tty_channel*)malloc(sizeof(struct tty_channel));
    if (!channel)
		return NULL;

	// Make a copy of config string, because we're about to mutate it
	input = strdup(input);
	if (!input)
		goto error;

	baud = DEFAULT_BAUD;
	device = DEFAULT_TTY_DEVICE;

	// Parse command-line parameters
	// Command line looks like name1=value1,name2,name3=value3,...
	while ((param_name = strtok((char*)input, ","))) {

		input = NULL;

		// Parse a parameter's name and value
		param_value = strchr(param_name,'=');
		if (param_value != NULL) {
			*param_value = '\0';
			param_value++;		// Advance past '=' character
		}

		if (!strcmp(param_name, "baud") && param_value) {
			baud = parse_baud(param_value);
			if (baud == B0) {
				goto error;
			}
		} else if (!strcmp(param_name, "device")) {
			device = param_value;
		} else {
			fprintf(stderr, "error: unknown tty channel parameter \"%s\"\n", param_name);
			goto error;
		}
	}

	fd = open(device, O_RDWR);
	if (fd < 0)
		goto error;

    // Get the current options for the port...
    if (tcgetattr(fd, &options) < 0)
		goto error;

    // Set the serial baud rate
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // Enable the receiver and set local mode...

    /* cfmakeraw(&options);
     * 
     * cygwin lacks cfmakeraw(), just do it explicitly
     */    
    options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                	       |INLCR|IGNCR|ICRNL|IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    options.c_cflag &= ~(CSIZE|PARENB);
    options.c_cflag |= CS8;
   
    options.c_cflag |= (CLOCAL | CREAD);


    // Set the new options for the port...
    if (tcsetattr(fd, TCSANOW, &options) < 0)
		goto error;

    channel->fds.fdin = channel->fds.fdout = fd;
	free((void *)input);
    return channel;
    
error:
    if (fd > 0)
		close(fd);
    free(channel);
	if (input)
		free((void *)input);
    return NULL;
}

static int tty_open(void * data)
{
    return 0;
}

struct channel_ops tty_channel_ops =
{
    init:   tty_init,
    open:   tty_open,
    close:  generic_close,
    read:   fd_read,
    write:  fd_write,
    free:   generic_free,
};

/*
 * Local variables:
 * c-file-style: "linux"
 * End:
 */

