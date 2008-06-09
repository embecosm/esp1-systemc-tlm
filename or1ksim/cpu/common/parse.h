/* parse.h -- Header file for parse.c
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org

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

/* Here we define some often used caharcters in assembly files.
This wil probably go into architecture dependent directory. */

#define COMMENT_CHAR	'#'
#define DIRECTIVE_CHAR	'.'
#define LABELEND_CHAR	":"
/*#define OPERAND_DELIM	","*/

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
#ifndef whitespace
#define	whitespace(a)	((a) == '\t' ? 1 : ((a) == ' '? 1 : 0))
#endif
   
/* Strips white spaces at beginning and at the end of the string */
char *stripwhite (char *string);

/* This function is very similar to strncpy, except it null terminates the string */
char *strstrip (char *dst, const char *src, int n);

/* Loads file to memory starting at address startaddr and returns freemem. */
uint32_t loadcode(char *filename, oraddr_t startaddr, oraddr_t virtphy_transl);
