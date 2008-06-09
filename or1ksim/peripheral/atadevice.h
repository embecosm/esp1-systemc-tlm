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
 * Definitions for the Opencores ATA Controller Core, Device model
 */

#ifndef __OR1KSIM_ATAD_H
#define __OR1KSIM_ATAD_H

#include <stdio.h>

/* --- Register definitions --- */
/* ----- ATA Registers                                                */
/* These are actually the memory locations where the ATA registers    */
/* can be found in the host system; i.e. as seen from the CPU.        */
/* However, this doesn't matter for the simulator.                    */
#define ATA_ASR   0x78         /* Alternate Status Register      (R)  */
#define ATA_CR    0x5c         /* Command Register               (W)  */
#define ATA_CHR   0x54         /* Cylinder High Register       (R/W)  */
#define ATA_CLR   0x50         /* Cylinder Low Register        (R/W)  */
#define ATA_DR    0x40         /* Data Register                       */
#define ATA_DCR   0x78         /* Device Control Register        (W)  */
#define ATA_DHR   0x58         /* Device/Head Register         (R/W)  */
#define ATA_ERR   0x44         /* Error Register                 (R)  */
#define ATA_FR    0x44         /* Features Register              (W)  */
#define ATA_SCR   0x48         /* Sector Count Register        (R/W)  */
#define ATA_SNR   0x4c         /* Sector Number Register       (R/W)  */
#define ATA_SR    0x5c         /* Status Register                (R)  */
#define ATA_DA    0x7c         /* Device Address Register        (R)  */
             /* ATA/ATAPI-5 does not describe Device Status Register  */

/* --------------------------------------                             */
/* ----- ATA Device bit defenitions -----                             */
/* --------------------------------------                             */

/* ----- ATA (Alternate) Status Register                              */
#define ATA_SR_BSY  0x80        /* Busy                               */
#define ATA_SR_DRDY 0x40        /* Device Ready                       */
#define ATA_SR_DF   0x20        /* Device Fault                       */
#define ATA_SR_DSC  0x10        /* Device Seek Complete               */
#define ATA_SR_DRQ  0x08        /* Data Request                       */
#define ATA_SR_COR  0x04        /* Corrected data (obsolete)          */
#define ATA_SR_IDX  0x02        /*                (obsolete)          */
#define ATA_SR_ERR  0x01        /* Error                              */

/* ----- ATA Device Control Register                                  */
                                /* bits 7-3 are reserved              */
#define ATA_DCR_RST 0x04        /* Software reset   (RST=1, reset)    */
#define ATA_DCR_IEN 0x02        /* Interrupt Enable (IEN=0, enabled)  */
                                /* always write a '0' to bit0         */

/* ----- ATA Device Address Register                                  */
/* All values in this register are one's complement (i.e. inverted)   */
#define ATA_DAR_WTG 0x40        /* Write Gate                         */
#define ATA_DAR_H   0x3c        /* Head Select                        */
#define ATA_DAR_DS1 0x02        /* Drive select 1                     */
#define ATA_DAR_DS0 0x01        /* Drive select 0                     */

/* ----- Device/Head Register                                         */
#define ATA_DHR_LBA 0x40        /* LBA/CHS mode ('1'=LBA mode)        */
#define ATA_DHR_DEV 0x10        /* Device       ('0'=dev0, '1'=dev1)  */
#define ATA_DHR_H   0x0f        /* Head Select                        */

/* ----- Error Register                                               */
#define ATA_ERR_BBK  0x80       /* Bad Block                          */
#define ATA_ERR_UNC  0x40       /* Uncorrectable Data Error           */
#define ATA_ERR_IDNF 0x10       /* ID Not Found                       */
#define ATA_ERR_ABT  0x04       /* Aborted Command                    */
#define ATA_ERR_TON  0x02       /* Track0 Not Found                   */
#define ATA_ERR_AMN  0x01       /* Address Mark Not Found             */

/* --------------------------                                         */
/* ----- Device Defines -----                                         */
/* --------------------------                                         */

/* types for hard disk simulation                                     */
#define TYPE_NO_CONNECT 0
#define TYPE_FILE       1
#define TYPE_LOCAL      2


/* -----------------------------                                      */
/* ----- Statemachine defines --                                      */
/* -----------------------------                                      */
#define ATA_STATE_IDLE   0x00
#define ATA_STATE_SW_RST 0x01
#define ATA_STATE_HW_RST 0x02


/* ----------------------------                                       */
/* ----- Structs          -----                                       */
/* ----------------------------                                       */
typedef struct{

        /******* Housekeeping *****************************************/
	struct {
		/* device number                                      */
		int dev;

		/* current PIO mode                                   */
		int pio_mode;

		/* current DMA mode                                   */
		int dma_mode;

		/* databuffer                                         */
	        unsigned short dbuf[4096];
		unsigned short *dbuf_ptr;
		unsigned short dbuf_cnt;

		/* current statemachine state                         */
		int state;

		/* current CHS translation settings                   */
		unsigned int heads_per_cylinder;
		unsigned int sectors_per_track;
	} internals;
	

	/******* ATA Device Registers *********************************/
	struct {
		unsigned char command;
		unsigned char cylinder_low;
		unsigned char cylinder_high;
		unsigned char device_control;
		unsigned char device_head;
		unsigned char error;
		unsigned char features;
		unsigned char sector_count;
		unsigned char sector_number;
		unsigned char status;

		short dataport_i;
	} regs;

	/******** ata device output signals **************************/
	struct {
		int iordy;
		int intrq;
		int dmarq;
		int pdiagi, pdiago;
		int daspi, daspo;
	} sigs;

	/******** simulator settings **********************************/
	/* simulate ata-device                                        */
	char *file;   /* Filename (if type == FILE)                   */
	FILE *stream; /* stream where the simulated device connects to*/
	int  type;    /* Simulate device using                        */
               	      /* NO_CONNECT: no device connected (dummy)      */
                      /* FILE      : a file                           */
       	              /* LOCAL     : a local stream, e.g./dev/hda1    */
	unsigned long size;    /* size in MB of the simulated device  */
	int  packet;  /* device implements PACKET command set         */
} ata_device;

typedef struct{
  ata_device device0, device1;
} ata_devices;


/* all devices                                                        */
void  ata_devices_init(ata_devices *devices);
void  ata_devices_hw_reset(ata_devices *devices, int reset_signal);
short ata_devices_read(ata_devices *devices, char adr);
void  ata_devices_write(ata_devices *devices, char adr, short value);

/* single device                                                      */
void ata_device_init(ata_device *device, int dev);
void ata_device_hw_reset(ata_device *device, int reset_signal, int daspo, int pdiagi, int daspi);
void ata_device_do_control_register(ata_device *device);
void ata_device_do_command_register(ata_device *device);
void ata_device_write(ata_device *device, char adr, short value);

/* housekeeping routines                                              */
FILE  *open_file(unsigned long *size, const char *filename);
FILE  *open_local(void);


#endif
