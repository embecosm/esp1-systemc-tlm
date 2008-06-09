/* tick.h -- Definition of types and structures for openrisc 1000 tick timer
   Copyright (C) 2000 Damjan Lampret, lampret@opencores.org

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

/* Reset the tick counter */
void tick_reset();

/* Starts the tick timer.  These functions are called by a write to one of the
 * timer sprs */
void spr_write_ttcr (uorreg_t value);
void spr_write_ttmr (uorreg_t value);

uorreg_t spr_read_ttcr (void);
