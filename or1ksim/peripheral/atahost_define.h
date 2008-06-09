/*
    atahost.h -- ATA Host code simulation
    Copyright (C) 2002 Richard Herveille, rherveille@opencores.org

    This file is part of OpenRISC 1000 Architectural Simulator

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * User configuration of the OCIDEC ata core
 */

#ifndef __OR1KSIM_ATAC_H
#define __OR1KSIM_ATAC_H


/* define core (OCIDEC) type */
#define DEV_ID 1

/* define core version */
#define REV 0

/* define timing reset values */
#define PIO_MODE0_T1 6
#define PIO_MODE0_T2 28
#define PIO_MODE0_T4 2
#define PIO_MODE0_TEOC 23

#define DMA_MODE0_TM 4
#define DMA_MODE0_TD 21
#define DMA_MODE0_TEOC 21


#endif
