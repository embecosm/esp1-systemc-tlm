/*
    atadevice.c -- ATA Device simulation
    Simulates a harddisk, using a local streams.
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

#include <string.h>
#include <stdarg.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "abstract.h"

#include "atadevice.h"
#include "atacmd.h"
#include "sim-config.h"
#include "atadevice_cmdi.h"

#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ata);

/*
 mandatory commands:
 - execute device diagnostics       (done)
 - flush cache
 - identify device                  (done)
 - initialize device parameters     (done)
 - read dma
 - read multiple
 - read sector(s)                   (done)
 - read verify sector(s)
 - seek
 - set features
 - set multiple mode
 - write dma
 - write multiple
 - write sector(s)


 optional commands:
 - download microcode
 - nop
 - read buffer
 - write buffer


 prohibited commands:
 - device reset
*/


/*
  D E V I C E S _ I N I T
*/
void ata_devices_init(ata_devices *devices)
{
  ata_device_init(&devices->device0, 0);

  if (devices->device0.type)
    ata_device_init(&devices->device1, ATA_DHR_DEV);
  else
    ata_device_init(&devices->device1,           0);
}


void ata_device_init(ata_device *device, int dev)
{
  /* set DeviceID                                                     */
  device->internals.dev = dev;

  /* generate stream for hd_simulation                                */
  switch(device->type)
  {
    case TYPE_NO_CONNECT:
      TRACE("ata_device, using type NO_CONNECT.\n");
      device->stream = NULL;
      break;

    case TYPE_FILE:
      TRACE("ata_device, using device type FILE.\n");
      device->stream = open_file(&device->size, device->file);
      break;

    case TYPE_LOCAL:
      TRACE("ata_device, using device type LOCAL.\n");
      device->stream = open_local();
      break;

    default:
      ERR("Illegal device-type.  Defaulting to type NO_CONNECT.\n");
      device->stream = NULL;
      break;
  }
}

/* Use a file to simulate a hard-disk                                 */
FILE *open_file(unsigned long *size, const char *filename)
{
   FILE *fp;
   unsigned long  n;

   // TODO:

   /* check if a file with name 'filename' already exists             */
   if ( !(fp = fopen(filename, "rb+")) )
     if ( !(fp = fopen(filename, "wb+")) )
     {
       ERR( "ata_open_file, cannot open hd-file %s\n", filename );
       return NULL;
     }
     else
     {
       /* TODO create a file 'size' large */
       /* create a file 'size' large                                  */
       TRACE("ata_device; generating a new hard disk file.\n");
       TRACE("This may take a while, depending on the requested size.\n");
       for (n=0; n < (*size << 20); n++)
         fputc(0, fp);
     }
   else /* file already exist                                         */
     fprintf(stderr, "file %s already exists. Using existing file.\n", filename);


   TRACE("requested filesize was: %ld (MBytes).\n", *size);

   /* get the size of the file. This is also the size of the harddisk*/
   fseek(fp, 0, SEEK_END);
   *size = ftell(fp);

   TRACE("actual filesize is: %ld (MBytes).\n", *size >> 20);

   return fp;
}


/* Use a the local filesystem as a hard-disk                          */
FILE *open_local(void)
{
 // TODO:
 FIXME("Device type LOCAL is not yet supported. Defaulting to device type NO_CONNECT.");
 return NULL;
}




/*
  D E V I C E S _ H W _ R E S E T
*/
/* power-on and hardware reset                                        */
void ata_devices_hw_reset(ata_devices *devices, int reset_signal)
{
  /* display debug information                                        */
  TRACE("ata_devices_hw_reset.\n");

  /* find device 0                                                    */
  if ( (devices->device0.stream) && (devices->device1.stream) )
  {
    /* this one is simple, device0 is device0                         */

    /* 1) handle device1 first                                        */
    ata_device_hw_reset(&devices->device1, reset_signal,
			1,   /* assert dasp, this is device1          */
			0,   /* negate pdiag input, no more devices   */
			0);  /* negate dasp input, no more devices    */

    /* 2) Then handle device0                                         */
    ata_device_hw_reset(&devices->device0, reset_signal,
			0,
			devices->device1.sigs.pdiago,
			devices->device1.sigs.daspo);
  }
  else if (devices->device0.stream)
  {
    /* device0 is device0, there's no device1                         */
    ata_device_hw_reset(&devices->device0, reset_signal,
			0,   /* negate dasp, this is device0          */
			0,   /* negate pdiag input, there's no device1*/
			0);  /* negate dasp input, there's no device1 */
  }
  else if (devices->device1.stream)
  {
    /* device1 is (logical) device0, there's no (physical) device0    */
    ata_device_hw_reset(&devices->device1, reset_signal,
			0,   /* negate dasp, this is device0          */
			0,   /* negate pdiag input, there's no device1*/
			0);  /* negate dasp input, there's no device1 */
  }
  else
  {
    /* no devices connected                                           */
    TRACE("ata_device_hw_reset, no devices connected.\n");
  }
}

void ata_device_hw_reset(ata_device *device, int reset_signal,
  int daspo, int pdiagi, int daspi)
{
    /* check ata-device state                                         */
    if (device->internals.state == ATA_STATE_HW_RST)
    {
      if (!reset_signal)
      {
        /* hardware reset finished                                    */

	/* set sectors_per_track & heads_per_cylinders                */
	device->internals.sectors_per_track = SECTORS;
	device->internals.heads_per_cylinder = HEADS;

	/* set device1 input signals                                  */
	device->sigs.pdiagi = pdiagi;
	device->sigs.daspi  = daspi;

        ata_execute_device_diagnostics_cmd(device);

        /* clear busy bit                                             */
        device->regs.status &= ~ATA_SR_BSY;

        /* set DRDY bit, when not a PACKET device                     */
        if (!device->packet)
            device->regs.status |= ATA_SR_DRDY;

	/* set new state                                              */
	device->internals.state = ATA_STATE_IDLE;
      }
    }
    else
      if (reset_signal)
      {
        /* hardware reset signal asserted, stop what we are doing     */

        /* negate bus signals                                         */
        device->sigs.iordy  = 0;
        device->sigs.intrq  = 0;
        device->sigs.dmarq  = 0;
        device->sigs.pdiago = 0;
        device->sigs.daspo  = daspo;

        /* set busy bit                                               */
        device->regs.status |= ATA_SR_BSY;
	PRINTF("setting status register BSY 0x%2X\n", device->regs.status);

        /* set new state                                              */
        device->internals.state = ATA_STATE_HW_RST;
      }
}


/*
  D E V I C E S _ D O _ C O N T R O L _ R E G I S T E R

  Handles - software reset
          - Interrupt enable bit
*/
void ata_device_do_control_register(ata_device *device)
{
    /* TODO respond to new values of nIEN */
    /* if device not selected, respond to new values of nIEN & SRST */

  /* check if SRST bit is set                                         */
  if (device->regs.device_control & ATA_DCR_RST)
  {
    if (device->internals.state == ATA_STATE_IDLE)
    {   /* start software reset                                       */
        /* negate bus signals                                         */
        device->sigs.pdiago = 0;
        device->sigs.intrq  = 0;
        device->sigs.iordy  = 0;
        device->sigs.dmarq  = 0;

        /* set busy bit                                               */
        device->regs.status |= ATA_SR_BSY;

        /* set new state                                              */
        device->internals.state = ATA_STATE_SW_RST;

	/* display debug information                                  */
        TRACE("ata_device_sw_reset initiated.\n");
    }
  }
  else if (device->internals.state == ATA_STATE_SW_RST)
  {   /* are we doing a software reset ??                             */
      /* SRST bit cleared, end of software reset                      */

      /*execute device diagnostics                                    */
      ata_execute_device_diagnostics_cmd(device);

      /* clear busy bit                                               */
      device->regs.status &= ~ATA_SR_BSY;

      /* set DRDY bit, when not a PACKET device                       */
      if (!device->packet)
        device->regs.status |= ATA_SR_DRDY;

      /* set new state                                                */
      device->internals.state = ATA_STATE_IDLE;

      /* display debug information                                    */
      TRACE("ata_device_sw_reset done.\n");
  }
  /*
  <else> We are doing a hardware reset (or similar)
         ignore command
  */
}


/*
  D E V I C E S _ D O _ C O M M A N D _ R E G I S T E R
*/
void ata_device_do_command_register(ata_device *device)
{
  /* check BSY & DRQ                                                  */
  if ( (device->regs.status & ATA_SR_BSY) || (device->regs.status & ATA_SR_DRQ) )
     if (device->regs.command != DEVICE_RESET)
        WARN("ata_device_write, writing a command while BSY or DRQ asserted.");

  /* check if device selected                                         */
  if ( (device->regs.device_head & ATA_DHR_DEV) == device->internals.dev )
      ata_device_execute_cmd(device);
  else
  {
      /* if not selected, only respond to EXECUTE DEVICE DIAGNOSTICS  */
      if (device->regs.command == EXECUTE_DEVICE_DIAGNOSTICS)
          ata_device_execute_cmd(device);
  }
}


/*
  D E V I C E S _ R E A D
*/
/* Read from devices                                                  */
short ata_devices_read(ata_devices *devices, char adr)
{
    ata_device *device;

    /* check for no connected devices                                 */
    if ( (!devices->device0.stream) && (!devices->device1.stream) )
        ERR("ata_devices_read, no ata devices connected.\n");
    else
    {
      /* check if both device0 and device1 are connected              */
      if ( (devices->device0.stream) && (devices->device1.stream) )
      {
          /* get the current active device                            */
          if (devices->device1.regs.device_head & ATA_DHR_DEV)
              device = &devices->device1;
          else
              device = &devices->device0;
      }
      else
      {
          /* only one device connected                                */
          if (devices->device1.stream)
              device = &devices->device1;
          else
              device = &devices->device0;
      }

      /* return data provided by selected device                      */
      switch (adr) {
        case ATA_ASR :
          TRACE("alternate_status register read\n");
	  if ( (device->regs.device_head & ATA_DHR_DEV) ==  device->internals.dev )
              return device -> regs.status;
	  else
	  {
	      TRACE("device0 responds for device1, asr = 0x00\n");
	      return 0; // return 0 when device0 responds for device1
	  }

        case ATA_CHR :
          TRACE("cylinder_high register read, value = 0x%02X\n",
	        device->regs.cylinder_high);
          return device -> regs.cylinder_high;

        case ATA_CLR :
          TRACE("cylinder_low register read, value = 0x%02X\n",
	         device->regs.cylinder_low);
          return device -> regs.cylinder_low;

        case ATA_DR  :
	  if (!device->regs.status & ATA_SR_DRQ)
	  {
	     TRACE("data register read, while DRQ bit negated\n" );
	     return 0;
	  }
	  else
	  {
              TRACE("data register read, value = 0x%04X, cnt = %3d\n",
	            *device->internals.dbuf_ptr, device->internals.dbuf_cnt);
              if (!--device->internals.dbuf_cnt)
                   device->regs.status &= ~ATA_SR_DRQ;
	      return *device -> internals.dbuf_ptr++;
	  }

        case ATA_DHR :
          TRACE("device_head register read, value = 0x%02X\n",
	         device->regs.device_head);
          return device -> regs.device_head;

        case ATA_ERR :
          TRACE("error register read, value = 0x%02X\n",
	         device->regs.error);
          return device -> regs.error;

        case ATA_SCR :
          TRACE("sectorcount register read, value = 0x%02X\n",
	         device->regs.sector_count);
          return device -> regs.sector_count;

        case ATA_SNR :
          TRACE("sectornumber register read, value = 0x%02X\n",
	         device->regs.sector_number);
          return device -> regs.sector_number;

        case ATA_SR  :
          TRACE("status register read\n");
	  if ( (device->regs.device_head & ATA_DHR_DEV) ==  device->internals.dev)
              return device -> regs.status;
	  else
	  {
	      TRACE("device0 responds for device1, sr = 0x00\n");
	      return 0; // return 0 when device0 responds for device1
	  }

//        case ATA_DA   :
//          return device -> regs.status;
      }
    }
    return 0;
}


/*
  D E V I C E S _ W R I T E
*/
/* Write to devices                                                   */
void ata_devices_write(ata_devices *devices, char adr, short value)
{
    /* check for no connected devices                                 */
    if (!devices->device0.stream && !devices->device1.stream)
        ERR("ata_devices_write, no ata devices connected.\n");
    else
    {
      /* first device                                                 */
      if (devices->device0.stream)
          ata_device_write(&devices->device0, adr, value);

      /* second device                                                */
      if (devices->device1.stream)
          ata_device_write(&devices->device1, adr, value);
    }
}

/* write to a single device                                           */
void ata_device_write(ata_device *device, char adr, short value)
{
    switch (adr) {
        case ATA_CR  :
	    /*display debug information                               */
	    TRACE("command register written, value = 0x%02X\n", value);

            device->regs.command = value;
	    
	    /* check command register settings and execute command    */
	    ata_device_do_command_register(device);
            break;


        case ATA_CHR :
	    /*display debug information                               */
	    TRACE("cylinder high register written, value = 0x%02X\n", value);

            device->regs.cylinder_high = value;
            break;

        case ATA_CLR :
	    /*display debug information                               */
	    TRACE("cylinder low register written, value = 0x%02X\n", value);

            device->regs.cylinder_low = value;
            break;

        case ATA_DR :
	    /*display debug information                               */
	    TRACE("data register written, value = 0x%04X\n", value);

            device->regs.dataport_i = value;
            break;

        case ATA_DCR :
	    /*display debug information                               */
	    TRACE("device control register written, value = 0x%02X\n", value);

            device->regs.device_control = value;
            ata_device_do_control_register(device);
	    break;

        case ATA_DHR :
	    /*display debug information                               */
	    TRACE("device head register written, value = 0x%02X\n", value);

            device->regs.device_head = value;
            break;

        case ATA_FR  :
	    /*display debug information                               */
	    TRACE("features register written, value = 0x%02X\n", value);

            device->regs.features = value;
            break;

        case ATA_SCR :
	    /*display debug information                               */
	    TRACE("sectorcount register written, value = 0x%02X\n", value);

            device->regs.sector_count = value;
            break;

        case ATA_SNR :
	    /*display debug information                               */
	    TRACE("sectornumber register written, value = 0x%02X\n", value);

            device->regs.sector_number = value;
            break;

    } //endcase
}

