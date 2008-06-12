/* generic.h -- Generic external peripheral
 *
 * Copyright (C) 2008 Jeremy Bennett, jeremy@jeremybennett.com
 *
 * This file is part of OpenRISC 1000 Architectural Simulator.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id*
 *
 */


/* State associated with the generic device. */

struct dev_generic {

  /* Info about a particular transaction */

  enum {				/* Direction of the access */
    GENERIC_READ,
    GENERIC_WRITE
  } trans_direction;

  enum {				/* Size of the access */
    GENERIC_BYTE,
    GENERIC_HW,
    GENERIC_WORD
  } trans_size;

  uint32_t  value;			/* The value to read/write */

  /* Configuration */

  int       enabled;			/* Device enabled */
  int       byte_enabled;		/* Byte R/W allowed */
  int       hw_enabled;			/* Half word R/W allowed */
  int       word_enabled;		/* Full word R/W allowed */
  char     *name;			/* Name of the device */
  oraddr_t  baseaddr;			/* Base address of device */
  uint32_t  size;			/* Address space size (bytes) */

};
