/*
    atadevice_cmdi.c -- ATA Device simulation
    Command interpreter for the simulated harddisk.
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
 For fun, count the number of FIXMEs :-(
*/

#include <string.h>

#include "atadevice.h"
#include "atadevice_cmdi.h"
#include "atacmd.h"

#include "debug.h"

DEFAULT_DEBUG_CHANNEL(ata);

int ata_device_execute_cmd(ata_device *device)
{
  /*display debug information                                         */
  TRACE("ata_device_execute_command called with command = 0x%02X\n",
        device->regs.command);

  /* execute the commands */
  switch (device->regs.command) {
      
      case DEVICE_RESET :
         ata_device_reset_cmd(device);
	 return 0;

      case EXECUTE_DEVICE_DIAGNOSTICS :
        ata_execute_device_diagnostics_cmd(device);
	return 0;

      case IDENTIFY_DEVICE :
        ata_identify_device_cmd(device);
	return 0;

      case INITIALIZE_DEVICE_PARAMETERS :
        ata_initialize_device_parameters_cmd(device);
	return 0;

      case READ_SECTORS :
        ata_read_sectors_cmd(device);

      default :
        return -1;
  }
}



/*
  A T A _ S E T _ D E V I C E _ S I G N A T U R E

  called whenever a command needs to signal the device type (packet, non-packet)
*/
void ata_set_device_signature(ata_device *device, int signature)
{
    if (signature)
    {
      /* place PACKET Command feature set signature in register block   */
      device->regs.sector_count  = 0x01;
      device->regs.sector_number = 0x01;
      device->regs.cylinder_low  = 0x14;
      device->regs.cylinder_high = 0xEB;
    }
    else
    {
      /* place NON_PACKET Command feature set signature in register block */
      device->regs.sector_count  = 0x01;
      device->regs.sector_number = 0x01;
      device->regs.cylinder_low  = 0x00;
      device->regs.cylinder_high = 0x00;
      device->regs.device_head   = 0x00;
    }
}


/*
  A T A _ D E V I C E _ R E S E T
*/
void ata_device_reset_cmd(ata_device *device)
{
  /* print debug information                                          */
  TRACE("executing command 'device reset'\n");

  if (!device->packet)
      WARN("executing DEVICE_RESET on non-packet device.");

  ata_execute_device_diagnostics_cmd(device);
}


/*
  A T A _ E X E C U T E _ D E V I C E _ D I A G N O S T I C S
*/
void ata_execute_device_diagnostics_cmd(ata_device *device)
{
  /* print debug information                                          */
  TRACE("executing command 'execute_device_diagnostics'\n");

  /* clear SRST bit, if set                                           */
  device->regs.device_control &= ~ATA_DCR_RST;

  /* content of features register is undefined                        */

  /*
      set device error register
      Device0: 0x01 = Device0 passed & Device1 passed/not present
      Device1: 0x01 = Device1 passed
  */
  device->regs.error = 0x01;
  /* something about DASP- and error_register_bit7                    */
  /* if device1 is not present or if device1 is present and passed    */
  /* diagnostics, than bit7 should be cleared.                        */
  /* This means we always clear bit7                                  */


  /* check if device implements packet protocol                       */
  if (device->packet)
  {
    /* place PACKET command feature set signature in register block   */
    ata_set_device_signature(device, PACKET_SIGNATURE);

    device->regs.status &= 0xf8;

    if (device->regs.command == DEVICE_RESET)
      device->regs.device_head = device->regs.device_head & ATA_DHR_LBA;
    else
      device->regs.device_head = 0x00;
  }
  else
  {
    /* place NON_PACKET Command feature set signature in register block */
    ata_set_device_signature(device, !PACKET_SIGNATURE);

    device->regs.status &= 0x82;
  }


  /* we passed diagnostics, so set the PDIAG-line                     */
  device->sigs.pdiago = 1;
}



/*
  A T A _ I D E N T I F Y _ D E V I C E
*/
void ata_identify_device_cmd(ata_device *device)
{
  unsigned char  *chksum_buf, chksum;
  unsigned short *buf;
  int            n;
  unsigned int   tmp;

  /* print debug information                                          */
  TRACE("ata_device executing command 'identify device'\n");

  /* reset databuffer                                                 */
  device->internals.dbuf_cnt = 256;
  device->internals.dbuf_ptr = device->internals.dbuf;

  buf = device->internals.dbuf;
  chksum_buf = (unsigned char *)buf;

  /*
    if number_of_available_sectors (==device->size / BYTES_PER_SECTOR) < 16,514,064 then
       support CHS translation where:
    LBA = ( ((cylinder_number*heads_per_cylinder) +head_number) *sectors_per_track ) +sector_number -1;

    Cylinders : 1-255   (or max.cylinders)
    Heads     : 0-15    (or max.heads    )
    Sectors   : 0-65535 (or max.sectors  )
  */

  /*
   1) Word 1 shall contain the number of user-addressable logical cylinders in the default CHS
      translation. If the content of words (61:60) is less than 16,514,064 then the content of word 1
      shall be greater than or equal to one and less than or equal to 65,535. If the content of words
      (61:60) is greater than or equal to 16,514,064 then the content of
      word 1 shall be equal to 16,383.
   2) Word 3 shall contain the number of user-addressable logical heads in the default CHS
      translation. The content of word 3 shall be greater than or equal to one and less than or equal to
      16. For compatibility with some BIOSs, the content of word 3 may be equal to 15 if the content of
      word 1 is greater than 8192.
   3) Word 6 shall contain the number of user-addressable logical sectors in the default CHS
      translation. The content of word 6 shall be greater than or equal to one and less than or equal to
      63.
   4) [(The content of word 1) * (the content of word 3) * (the content of word 6)] shall be less than or
      equal to 16,514,064
   5) Word 54 shall contain the number of user-addressable logical cylinders in the current CHS
      translation. The content of word 54 shall be greater than or equal to one and less than or
      equal to 65,535. After power-on of after a hardware reset the content of word 54 shall be equal to
      the content of word 1.
   6) Word 55 shall contain the number of user-addressable logical heads in the current CHS
      translation. The content of word 55 shall be greater than or equal to one and less than or equal
      to 16. After power-on or after a hardware reset the content of word 55 shall equal the content of
      word 3.
   7) Word 56 shall contain the user-addressable logical sectors in the current CHS
      translation. The content of word 56 should be equal to 63 for compatibility with all BIOSs.
      However, the content of word 56 may be greater than or equal to one and less than or equal to
      255. At power-on or after a hardware reset the content of word 56 shall equal the content of
      word 6.
   8) Words (58:57) shall contain the user-addressable capacity in sectors for the current CHS
      translation. The content of words (58:57) shall equal [(the content of word 54) * (the content of
      word 55) * (the content of word 56)] The content of words (58:57) shall be less than or equal to
      16,514,064. The content of words (58:57) shall be less than or equal to the content of words
      (61:60).
   9) The content of words 54, 55, 56, and (58:57) may be affected by the host issuing an INITIALIZE
      DEVICE PARAMETERS command.
   10)If the content of words (61:60) is greater than 16,514,064 and if the device does not support CHS
      addressing, then the content of words 1, 3, 6, 54, 55, 56, and (58:57) shall equal zero. If the
      content of word 1, word 3, or word 6 equals zero, then the content of words 1, 3, 6, 54, 55, 56,
      and (58:57) shall equal zero.
   11)Words (61:60) shall contain the total number of user-addressable sectors. The content of words
      (61:60) shall be greater than or equal to one and less than or equal to 268,435,456.
   12)The content of words 1, 54, (58:57), and (61:60) may be affected by the host issuing a SET MAX
      ADDRESS command.
  */


  /* check if this is a NON-PACKET device                             */
  if (device->packet)
  {
    /*
      This is a PACKET device.
      Respond by placing PACKET Command feature set signature in block registers.
      Abort command.
    */
    TRACE("'identify_device' command: This is a PACKET device\n");

    ata_set_device_signature(device, PACKET_SIGNATURE);
    device->regs.status = ATA_SR_ERR;
    device->regs.error = ATA_ERR_ABT;
  }
  else
  {
    /* start filling buffer                                           */

    /*
       word 0: General configuration
               15  : 0 = ATA device
	       14-8: retired
	       7   : 1 = removable media device (not set)
	       6   : 1 = not-removable controller and/or device (set)
	       5-0 : retired and/or always set to zero
    */
    *buf++ = 0x0040;

    /*
       word 1: Number of logical cylinders

       >=1, <= 65535
    */
    if ( (tmp = device->size / BYTES_PER_SECTOR) >= 16514064 )
        *buf++ = 16383;
    else
      if ( (tmp /= (HEADS +1) * SECTORS) > 65535 )
          *buf++ = 65535;
      else
          *buf++ = tmp;

    /*
       word 2: Specific configuration

       0x37c8: Device requires SET_FEATURES subcommand to spinup after power-on
               and IDENTIFY_DEVICE response is incomplete
       0x738c: Device requires SET_FEATURES subcommand to spinup after power-on
               and IDENTIFY_DEVICE response is complete
       0x8c73: Device does not require SET_FEATURES subcommand to spinup after power-on
               and IDENTIFY_DEVICE response is incomplete
       0xc837: Device does not require SET_FEATURES subcommand to spinup after power-on
               and IDENTIFY_DEVICE response is complete

       pick-one
    */
    *buf++ = 0xc837;

    /*
       word 3: Number of logical heads
       
       >= 1, <= 16

       set to 15 if word1 > 8192 (BIOS compatibility)
    */
    *buf++ = HEADS;

    /*
       word 5-4: retired
    */
    buf += 2;

    /*
       word 6: Number of logical sectors per logical track
       
       >= 1, <= 63
    */
    *buf++ = SECTORS;

    /*
       word 8-7: Reserved by the CompactFLASH association
    */
    buf += 2;

    /*
       word 9: retired
    */
    buf++;

    /*
       word 19-10: Serial number (ASCII)
    */
    /*           " www.opencores.org  "                               */
    *buf++ = (' ' << 8) | 'w';
    *buf++ = ('w' << 8) | 'w';
    *buf++ = ('.' << 8) | 'o';
    *buf++ = ('p' << 8) | 'e';
    *buf++ = ('n' << 8) | 'c';
    *buf++ = ('o' << 8) | 'r';
    *buf++ = ('e' << 8) | 's';
    *buf++ = ('.' << 8) | 'o';
    *buf++ = ('r' << 8) | 'g';
    *buf++ = (' ' << 8) | ' ';

    /*
       word 22   : obsolete
       word 21-20: retired
    */
    buf += 3;

    /*
       word 26-23: Firmware revision
    */
    strncpy((char *)buf, FIRMWARE, 8);
    buf += 4;

    /*
       word 46-27: Model number
    */
    /*           " ata device model  (C)Richard Herveille "           */
    *buf++ = (' ' << 8) | 'a';
    *buf++ = ('t' << 8) | 'a';
    *buf++ = (' ' << 8) | 'd';
    *buf++ = ('e' << 8) | 'v';
    *buf++ = ('i' << 8) | 'c';
    *buf++ = ('e' << 8) | ' ';
    *buf++ = ('m' << 8) | 'o';
    *buf++ = ('d' << 8) | 'e';
    *buf++ = ('l' << 8) | ' ';
    *buf++ = (' ' << 8) | '(';
    *buf++ = ('C' << 8) | ')';
    *buf++ = ('R' << 8) | 'i';
    *buf++ = ('c' << 8) | 'h';
    *buf++ = ('a' << 8) | 'r';
    *buf++ = ('d' << 8) | ' ';
    *buf++ = ('H' << 8) | 'e';
    *buf++ = ('r' << 8) | 'v';
    *buf++ = ('e' << 8) | 'i';
    *buf++ = ('l' << 8) | 'l';
    *buf++ = ('e' << 8) | ' ';

    /*
       word 47:
           15-8: 0x80
	    7-0: 0x00 reserved
	         0x01-0xff maximum number of sectors to be transfered
		           per interrupt on a READ/WRITE MULTIPLE command
    */
    *buf++ = 0x8001;

    /*
       word 48: reserved
    */
    buf++;

    /*
       word 49: Capabilities
           15-14: reserved for IDENTIFY PACKET DEVICE
	   13   : 1=standby timers are supported
	          0=standby timers are handled by the device FIXME
           12   : reserved for IDENTIFY PACKET DEVICE
	   11   : 1=IORDY supported
	          0=IORDY may be supported
           10   : 1=IORDY may be disabled
	    9   : set to one
	    8   : set to one
	    7-0 : retired
    */
    *buf++ = 0x0f00;

    /*
       word 50: Capabilities
           15  : always cleared to zero
	   14  : always set to one
	   13-1: reserved
	   0   : 1=Device specific standby timer minimum value FIXME
    */
    *buf++ = 0x4000;

    /*
       word 52-51: obsolete
    */
    buf += 2;

    /*
       word 53:
           15-3: Reserved
	   2   : 1=value in word 88 is valid
	         0=value in word 88 is not valid
	   1   : 1=value in word 64-70 is valid
	         0=value in word 64-70 is not valid
	   0   : 1=value in word 54-58 is valid
	         0=value in word 54-58 is not valid
    */
    *buf++ = 0x0007;


    /*
       word 54: number of current logical cylinders (0-65535)
    */
    if ( (tmp = device->size / BYTES_PER_SECTOR) > 16514064 )
        tmp = 16514064;

    tmp /= (device->internals.heads_per_cylinder +1) * (device->internals.sectors_per_track);
    if (tmp > 65535)
        *buf++ = 65535;
    else
        *buf++ = tmp;

    /*
       word 55: number of current logical heads, (0-15)
    */
    *buf++ = device->internals.heads_per_cylinder +1;

    /*
       word 56: number of current logical sectors per track (1-255)
    */
    *buf++ = device->internals.sectors_per_track;

    /*
       word 58-57: current capacity in sectors
    */
    tmp = *(buf-3) * *(buf-2) * *(buf-1);
    *buf++ = tmp >> 16;
    *buf++ = tmp & 0xffff;

    /*
       word 59:
           15-9: reserved
	   8   : 1=multiple sector setting is valid
	   7-0 : current setting for number of sectors to be transfered
	         per interrupt on a READ/WRITE MULTIPLE command
    */
    *buf++ = 0x0001; // not really a FIXME

    /*
       word 60-61: Total number of user addressable sectors (LBA only)
    */
    *buf++ = (device->size / BYTES_PER_SECTOR);
    *buf++ = (device->size / BYTES_PER_SECTOR) >> 16;

    /*
       word 62: obsolete
    */
    buf++;

    /* FIXME
       word 63: DMA settings
           15-11: Reserved
	   10   : 1=Multiword DMA Mode 2 is selected
	          0=Multiword DMA Mode 2 is not selected
	    9   : 1=Multiword DMA Mode 1 is selected
	          0=Multiword DMA Mode 1 is not selected
	    8   : 1=Multiword DMA Mode 0 is selected
	          0=Multiword DMA Mode 0 is not selected
            7-3 : Reserved
	    2   : Multiword DMA Mode 2 and below are supported
	    1   : Multiword DMA Mode 1 and below are supported
	    0   : Multiword DMA Mode 0 is supported
    */
    #if   (MWDMA == -1)
      *buf++ = 0x0000;
    #elif (MWDMA == 2)
      *buf++ = 0x0007;
    #elif (MWDMA == 1)
      *buf++ = 0x0003;
    #else
      *buf++ = 0x0001;
    #endif

    /*
       word 64:
           15-8: reserved
	    7-0: Advanced PIO modes supported
	    
	    7-2: reserved
	    1  : PIO mode4 supported
	    0  : PIO mode3 supported
    */
    *buf++ = 0x0003;

    /*
       word 65: Minimum Multiword DMA transfer cycle time per word (nsec)
    */
    *buf++ = MIN_MWDMA_CYCLE_TIME;

    /*
       word 66: Manufacturer's recommended Multiword DMA transfer cycle time per word (nsec)
    */
    *buf++ = RECOMMENDED_MWDMA_CYCLE_TIME;


    /*
       word 67: Minimum PIO transfer cycle time per word (nsec), without IORDY flow control
    */
    *buf++ = MIN_PIO_CYCLE_TIME_NO_IORDY;

    /*
       word 68: Minimum PIO transfer cycle time per word (nsec), with IORDY flow control
    */
    *buf++ = MIN_PIO_CYCLE_TIME_IORDY;

    /*
       word 69-70: reserved for future command overlap and queueing
            71-74: reserved for IDENTIFY PACKET DEVICE command
    */
    buf += 6;

    /*
       word 75: Queue depth
           15-5: reserved
	    4-0: queue depth -1
    */
    *buf++ = SUPPORT_READ_WRITE_DMA_QUEUED ? QUEUE_DEPTH : 0x0000;

    /*
       word 76-79: reserved
    */
    buf += 4;

    /*
       word 80: MAJOR ATA version
                We simply report that we do not report a version
		
		You can also set bits 5-2 to indicate compliancy to
		ATA revisions 5-2 (1 & 0 are obsolete)
    */
    *buf++ = 0x0000;

    /*
       word 81: MINOR ATA version
                We report ATA/ATAPI-5 T13 1321D revision 3 (0x13)
    */
    *buf++ = 0x0013;

    /*
       word 82: Command set supported
    */
    *buf++ = 0                           << 15 |   /* obsolete        */
             SUPPORT_NOP_CMD             << 14 |
	     SUPPORT_READ_BUFFER_CMD     << 13 |
	     SUPPORT_WRITE_BUFFER_CMD    << 12 |
	     0                           << 11 |   /* obsolete        */
	     SUPPORT_HOST_PROTECTED_AREA << 10 |
	     SUPPORT_DEVICE_RESET_CMD    << 9  |
	     SUPPORT_SERVICE_INTERRUPT   << 8  |
	     SUPPORT_RELEASE_INTERRUPT   << 7  |
	     SUPPORT_LOOKAHEAD           << 6  |
	     SUPPORT_WRITE_CACHE         << 5  |
	     0                           << 4  |   /* cleared to zero */
	     SUPPORT_POWER_MANAGEMENT    << 3  |
	     SUPPORT_REMOVABLE_MEDIA     << 2  |
	     SUPPORT_SECURITY_MODE       << 1  |
	     SUPPORT_SMART               << 0
    ;

    /*
       word 83: Command set supported
    */
    *buf++ = 0                           << 15 |   /* cleared to zero */
             1                           << 14 |   /* set to one      */
	     0                           << 9  |   /* reserved        */
	     SUPPORT_SET_MAX             << 8  |
	     0                           << 7  |   /* reserved for    */
	                                           /* project 1407DT  */
	     SET_FEATURES_REQUIRED_AFTER_POWER_UP << 6  |
	     SUPPORT_POWER_UP_IN_STANDBY_MODE     << 5  |
	     SUPPORT_REMOVABLE_MEDIA_NOTIFICATION << 4  |
	     SUPPORT_APM                          << 3  |
	     SUPPORT_CFA                          << 2  |
	     SUPPORT_READ_WRITE_DMA_QUEUED        << 1  |
	     SUPPORT_DOWNLOAD_MICROCODE           << 0
    ;

    /*
       word 84: Command set/feature supported
    */
    *buf++ = 0                           << 15 |   /* cleared to zero */
             1                           << 14     /* set to one      */
    ;                                              /* 0-13 reserved   */

    /*
       word 85: Command set enabled FIXME
    */
    *buf++ = 0                           << 15 |   /* obsolete        */
             SUPPORT_NOP_CMD             << 14 |
	     SUPPORT_READ_BUFFER_CMD     << 13 |
	     SUPPORT_WRITE_BUFFER_CMD    << 12 |
	     0                           << 11 |   /* obsolete        */
	     SUPPORT_HOST_PROTECTED_AREA << 10 |
	     SUPPORT_DEVICE_RESET_CMD    << 9  |
	     SUPPORT_SERVICE_INTERRUPT   << 8  |
	     SUPPORT_RELEASE_INTERRUPT   << 7  |
	     SUPPORT_LOOKAHEAD           << 6  |
	     SUPPORT_WRITE_CACHE         << 5  |
	     0                           << 4  |   /* cleared to zero */
	     SUPPORT_POWER_MANAGEMENT    << 3  |
	     SUPPORT_REMOVABLE_MEDIA     << 2  |
	     SUPPORT_SECURITY_MODE       << 1  |
	     SUPPORT_SMART               << 0
    ;

    /*
       word 86: Command set enables
    */
    *buf++ = 0                           << 9  |   /* 15-9 reserved   */
	     SUPPORT_SET_MAX             << 8  |
	     0                           << 7  |   /* reserved for    */
	                                           /* project 1407DT  */
	     SET_FEATURES_REQUIRED_AFTER_POWER_UP << 6  |
	     SUPPORT_POWER_UP_IN_STANDBY_MODE     << 5  |
	     SUPPORT_REMOVABLE_MEDIA_NOTIFICATION << 4  |
	     SUPPORT_APM                          << 3  |
	     SUPPORT_CFA                          << 2  |
	     SUPPORT_READ_WRITE_DMA_QUEUED        << 1  |
	     SUPPORT_DOWNLOAD_MICROCODE           << 0
    ;

    /*
       word 87: Command set/feature supported
    */
    *buf++ = 0                           << 15 |   /* cleared to zero */
             1                           << 14     /* set to one      */
    ;                                              /* 0-13 reserved   */

    /*
       word 88: UltraDMA section
           15-13: reserved
	   12   : 1=UltraDMA Mode 4 is selected
	          0=UltraDMA Mode 4 is not selected
	   10   : 1=UltraDMA Mode 3 is selected
	          0=UltraDMA Mode 3 is not selected
	   10   : 1=UltraDMA Mode 2 is selected
	          0=UltraDMA Mode 2 is not selected
	    9   : 1=UltraDMA Mode 1 is selected
	          0=UltraDMA Mode 1 is not selected
	    8   : 1=UltraDMA Mode 0 is selected
	          0=UltraDMA Mode 0 is not selected
            7-5 : Reserved
	    4   : UltraDMA Mode 4 and below are supported
	    3   : UltraDMA Mode 3 and below are supported
	    2   : UltraDMA Mode 2 and below are supported
	    1   : UltraDMA Mode 1 and below are supported
	    0   : UltraDMA Mode 0 is supported

	    Because the OCIDEC cores currently do not support
	    UltraDMA we set all bits to zero
    */
    *buf++ = 0;

    /*
       word 89: Security sector erase unit completion time

       For now we report a 'value not specified'.
    */
    *buf++ = 0x0000; // not really a FIXME

    /*
       word 90: Enhanced security erase completion time

       For now we report a 'value not specified'.
    */
    *buf++ = 0x0000; // not really a FIXME

    /*
       word 91: Current advanced power management value

       For now set it to zero.
    */
    *buf++ = 0x0000; // FIXME

    /*
       word 92: Master Password Revision Code.
               We simply report that we do not support this.
    */
    *buf++ = 0x0000;

    /*
       word 93: Hardware reset result
    */
    if (device->internals.dev)
    {
      /* this is device1, clear device0 bits                                         */
      *buf++ = 0              << 15 |
               1              << 14 |
               0              << 13 |   /* CBLIBD level (1=Vih, 0=Vil)               */
                                        /* 12-8 Device 1 hardware reset result       */
               0              << 12 |   /* reserved                                  */
               device->sigs.pdiago << 11 | /* 1: Device1 did assert PDIAG               */
                                        /* 0: Device1 did not assert PDIAG           */
               3              <<  9 |   /* Device1 determined device number by       */
                                        /* 00: reserved                              */
                                        /* 01: a jumper was used                     */
                                        /* 10: the CSEL signal was used              */
                                        /* 11: some other or unknown method was used */
               1              << 8      /* set to one                                */
      ;
    }
    else
    { /* FIXME bit 6 */
      /* this is device0, clear device1 bits                                         */
      *buf++ = 0              << 7 |    /* reserved                                  */
               0              << 6 |    /* 1: Device0 responds for device 1          */
                                        /* 0: Device0 does not respond for device1   */
               device->sigs.daspi  << 5 |  /* 1: Device0 did detected DASP assertion    */
                                        /* 0: Device0 did not detect DASP assertion  */
               device->sigs.pdiagi << 4 |  /* Device0 did detect PDIAG assertion        */
                                        /* Device0 did not detect PDIAG assertion    */
               1              << 3 |    /* Device0 did pass diagnostics              */
               3              << 1 |    /* Device0 determined device number by       */
                                        /* 00: reserved                              */
                                        /* 01: a jumper was used                     */
                                        /* 10: the CSEL signal was used              */
                                        /* 11: some other or unknown method was used */
               1              << 0      /* set to one                                */
      ;
    }

    /*
       word 94-126: Reserved
    */
    buf += 33;

    /*
       word 127: Removable Media Status Notification feature set support
           15-2: reserved
	    1-0: 00 Removable media Status Notification not supported
	         01 Removable media Status Notification supported
		 10 reserved
		 11 reserved
    */
    *buf++ = SUPPORT_REMOVABLE_MEDIA_NOTIFICATION;

    /*
       word 128: Security status
           15-9: reserved
	   8   : Security level, 0=high, 1=maximum
	   7-6 : reserved
	   5   : 1=enhanced security erase supported
	   4   : 1=security count expired
	   3   : 1=security frozen
	   2   : 1=security locked
	   1   : 1=security enabled
	   0   : security supported

	   for now we do not support security, set is to zero
    */
    *buf++ = 0;

    /*
       word 129-159: Vendor specific
    */
    buf += 31;

    /*
       word 160: CFA power mode 1
           15  : Word 160 supported
	   14  : reserved
	   13  : CFA power mode 1 is required for one or more commands
	   12  : CFA power mode 1 disabled
	   11-0: Maximum current in mA
    */
    *buf++ = 0;

    /*
       word 161-175: Reserved for the CompactFLASH Association
    */
    buf += 15;

    /*
       word 176-254: reserved
    */
    buf += 79;

    /*
       word 255: Integrity word
           15-8: Checksum
	   7-0 : Signature
    */
    // set signature to indicate valid checksum
    *buf = 0x00a5;

    // calculate checksum
    chksum = 0;
    for (n=0; n < 511; n++)
      chksum += *chksum_buf++;

    *buf = ( (0-chksum) << 8) | 0x00a5;

    /* set status register bits                                       */
    device->regs.status = ATA_SR_DRDY | ATA_SR_DRQ;
  }
}


/*
  A T A _ I N I T I A L I Z E _ D E V I C E _ P A R A M E T E R S
*/
void ata_initialize_device_parameters_cmd(ata_device *device)
{
  /* print debug information                                          */
  TRACE("executing command 'initialize device parameters'\n");

  device->internals.sectors_per_track = device->regs.sector_count;
  device->internals.heads_per_cylinder = device->regs.device_head & ATA_DHR_H;

  /* set status register bits                                         */
  device->regs.status = 0;
}


/*
  A T A _ R E A D _ S E C T O R S
*/
void ata_read_sectors_cmd(ata_device *device)
{
  size_t sector_count;
  unsigned long lba;

  /* print debug information                                          */
  TRACE("executing command 'read sectors'\n");

  /* check if this is a NON-PACKET device                             */
  if (device->packet)
  {
    /*
      This is a PACKET device.
      Respond by placing PACKET Command feature set signature in block registers.
      Abort command.
    */
    TRACE("'identify_device' command: This is a PACKET device\n");

    ata_set_device_signature(device, PACKET_SIGNATURE);
    device->regs.status = ATA_SR_ERR;
    device->regs.error = ATA_ERR_ABT;
  }
  else
  {
    /* get the sector count                                           */
    if (device->regs.sector_count == 0)
        sector_count = 256;
    else
        sector_count = device->regs.sector_count;

    /* check if we are using CHS or LBA translation, fill in the bits */
    if (device->regs.device_head & ATA_DHR_LBA)
    {   /* we are using LBA translation                               */
        lba = (device->regs.device_head & ATA_DHR_H) << 24 |
	      (device->regs.cylinder_high          ) << 16 |
	      (device->regs.cylinder_low           ) <<  8 |
	       device->regs.sector_number
        ;
    }
    else
    {   /* we are using CHS translation, calculate lba address        */
        lba  = (device->regs.cylinder_high << 16) | device->regs.cylinder_low;
	lba *= device->internals.heads_per_cylinder;
	lba += device->regs.device_head & ATA_DHR_H;
	lba *= device->internals.sectors_per_track;
	lba += device->regs.sector_number;
	lba -= 1;
    }

    /* check if sector within bounds                                  */
    if (lba > (device->size / BYTES_PER_SECTOR) )
    {   /* invalid sector address                                     */
        /* set the status & error registers                           */
	device->regs.status = ATA_SR_DRDY | ATA_SR_ERR;
	device->regs.error = ATA_ERR_IDNF;
    }
    else
    {   /* go ahead, read the bytestream                              */
        lba *= BYTES_PER_SECTOR;

        /* set the file-positon pointer to the start of the sector    */
	fseek(device->stream, lba, SEEK_SET);

	/* get the bytes from the stream                              */
	fread(device->internals.dbuf, BYTES_PER_SECTOR, sector_count, device->stream);

        /* set status register bits                                   */
        device->regs.status = ATA_SR_DRDY | ATA_SR_DRQ;
	
        /* reset the databuffer                                       */
        device->internals.dbuf_cnt = sector_count * BYTES_PER_SECTOR /2; //Words, not bytes
        device->internals.dbuf_ptr = device->internals.dbuf;
    }
  }
}


