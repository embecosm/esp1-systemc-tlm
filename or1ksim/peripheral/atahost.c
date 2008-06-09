/*
    atahost.c -- ATA Host code simulation
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

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
/* get a prototype for 'reg_mem_area()', and 'adjust_rw_delay()' */
#include "abstract.h"
#include "sim-config.h"
#include "sched.h"

/* all user defineable settings are in 'atahost_define.h'             */
#include "atahost_define.h"
#include "atahost.h"

/* reset and initialize ATA host core(s) */
void ata_reset(void *dat)
{
   ata_host *ata = dat;

   // reset the core registers
   ata->regs.ctrl  = 0x0001;
   ata->regs.stat  = (DEV_ID << 28) | (REV << 24);
   ata->regs.pctr  = (PIO_MODE0_TEOC << ATA_TEOC) | (PIO_MODE0_T4 << ATA_T4) | (PIO_MODE0_T2 << ATA_T2) | (PIO_MODE0_T1 << ATA_T1);
   ata->regs.pftr0 = (PIO_MODE0_TEOC << ATA_TEOC) | (PIO_MODE0_T4 << ATA_T4) | (PIO_MODE0_T2 << ATA_T2) | (PIO_MODE0_T1 << ATA_T1);
   ata->regs.pftr1 = (PIO_MODE0_TEOC << ATA_TEOC) | (PIO_MODE0_T4 << ATA_T4) | (PIO_MODE0_T2 << ATA_T2) | (PIO_MODE0_T1 << ATA_T1);
   ata->regs.dtr0  = (DMA_MODE0_TEOC << ATA_TEOC) | (DMA_MODE0_TD << ATA_TD) | (DMA_MODE0_TM << ATA_TM);
   ata->regs.dtr1  = (DMA_MODE0_TEOC << ATA_TEOC) | (DMA_MODE0_TD << ATA_TD) | (DMA_MODE0_TM << ATA_TM);
   ata->regs.txb   = 0;
     
   // inform simulator about new read/write delay timings
   adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.pctr), ata_pio_delay(ata->regs.pctr) );

   /* the reset bit in the control register 'ctrl' is set, reset connect ata-devices */
   ata_devices_hw_reset(&ata->devices, 1);
}
/* ========================================================================= */


/*
  Read a register
*/
uint32_t ata_read32( oraddr_t addr, void *dat )
{
    ata_host *ata = dat;

    /* determine if ata_host or ata_device addressed */
    if (is_ata_hostadr(addr))
    {
        // Accesses to internal register take 2cycles
        adjust_rw_delay( ata->mem, 2, 2 );

        switch( addr ) {
            case ATA_CTRL :
	        return ata -> regs.ctrl;

	    case ATA_STAT :
	        return ata -> regs.stat;

	    case ATA_PCTR :
	        return ata -> regs.pctr;

#if (DEV_ID > 1)
	    case ATA_PFTR0:
	        return ata -> regs.pftr0;

	    case ATA_PFTR1:
	        return ata -> regs.pftr1;
#endif

#if (DEV_ID > 2)
	    case ATA_DTR0 :
	        return ata -> regs.dtr0;

	    case ATA_DTR1 :
	        return ata -> regs.dtr1;

	    case ATA_RXB  :
	        return ata -> regs.rxb;
#endif

	    default:
		return 0;
        }
    }
    else
    /* check if the controller is enabled */
    if (ata->regs.ctrl & ATA_IDE_EN)
    {
        // make sure simulator uses correct read/write delay timings
#if (DEV_ID > 1)
        if ( (addr & 0x7f) == ATA_DR)
	{
	  if (ata->devices.dev)
	      adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.ftcr1), ata_pio_delay(ata->regs.ftcr1) );
	  else
	      adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.ftcr0), ata_pio_delay(ata->regs.ftcr0) );
	}
	else
#endif
        adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.pctr), ata_pio_delay(ata->regs.pctr) );

        return ata_devices_read(&ata->devices, addr & 0x7f);
    }
    return 0;
}
/* ========================================================================= */


/*
  Write a register
*/
void ata_write32( oraddr_t addr, uint32_t value, void *dat )
{
    ata_host *ata = dat;

    /* determine if ata_host or ata_device addressed */
    if (is_ata_hostadr(addr))
    {
       // Accesses to internal register take 2cycles
       adjust_rw_delay( ata->mem, 2, 2 );

        switch( addr ) {
            case ATA_CTRL :
	        ata -> regs.ctrl =  value;

		/* check if reset bit set, if so reset ata-devices    */
		if (value & ATA_RST)
		  ata_devices_hw_reset(&ata->devices, 1);
		else
		  ata_devices_hw_reset(&ata->devices, 0);
		break;

            case ATA_STAT :
		ata -> regs.stat = (ata -> regs.stat & ~ATA_IDEIS) | (ata -> regs.stat & ATA_IDEIS & value);
		break;

	    case ATA_PCTR :
	        ata -> regs.pctr = value;
		break;

	    case ATA_PFTR0:
	        ata -> regs.pftr0 = value;
		break;

	    case ATA_PFTR1:
	        ata -> regs.pftr1 = value;
		break;

	    case ATA_DTR0 :
	        ata -> regs.dtr0  = value;
		break;

	    case ATA_DTR1 :
	        ata -> regs.dtr1  = value;
		break;

	    case ATA_TXB  :
	        ata -> regs.txb   = value;
		break;

            default:
	        /* ERROR-macro currently only supports simple strings. */
		/*
	          fprintf(stderr, "ERROR  : Unknown register for OCIDEC(%1d).\n", DEV_ID );

		  Tried to show some useful info here.
		  But when using 'DM'-simulator-command, the screen gets filled with these messages.
		  Thereby eradicating the usefulness of the message
		*/
		break;
        }
    }
    else
    /* check if the controller is enabled */
    if (ata->regs.ctrl & ATA_IDE_EN)
    {
        // make sure simulator uses correct read/write delay timings
#if (DEV_ID > 1)
        if ( (addr & 0x7f) == ATA_DR)
	{
	  if (ata->devices.dev)
	      adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.ftcr1), ata_pio_delay(ata->regs.ftcr1) );
	  else
	      adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.ftcr0), ata_pio_delay(ata->regs.ftcr0) );
	}
	else
#endif
        adjust_rw_delay( ata->mem, ata_pio_delay(ata->regs.pctr), ata_pio_delay(ata->regs.pctr) );

        ata_devices_write(&ata->devices, addr & 0x7f, value);
    }
}
/* ========================================================================= */


/* Dump status */
void ata_status( void *dat )
{
  ata_host *ata = dat;

  if ( ata->baseaddr == 0 )
    return;

  PRINTF( "\nOCIDEC-%1d at: 0x%"PRIxADDR"\n", DEV_ID, ata->baseaddr );
  PRINTF( "ATA CTRL     : 0x%08X\n", ata->regs.ctrl  );
  PRINTF( "ATA STAT     : 0x%08x\n", ata->regs.stat  );
  PRINTF( "ATA PCTR     : 0x%08x\n", ata->regs.pctr  );

#if (DEV_ID > 1)
  PRINTF( "ATA FCTR0    : 0x%08lx\n", ata->regs.pftr0 );
  PRINTF( "ATA FCTR1    : 0x%08lx\n", ata->regs.pftr1 );
#endif

#if (DEV_ID > 2)
  PRINTF( "ATA DTR0     : 0x%08lx\n", ata->regs.dtr0  );
  PRINTF( "ATA DTR1     : 0x%08lx\n", ata->regs.dtr1  );
  PRINTF( "ATA TXD      : 0x%08lx\n", ata->regs.txb   );
  PRINTF( "ATA RXD      : 0x%08lx\n", ata->regs.rxb   );
#endif
}
/* ========================================================================= */

/*----------------------------------------------------[ ATA Configuration ]---*/
void ata_baseaddr(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->baseaddr = val.addr_val;
}

void ata_irq(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->irq = val.int_val;
}

void ata_dev_type0(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device0.type = val.int_val;
}

void ata_dev_file0(union param_val val, void *dat)
{
  ata_host *ata = dat;
  if(!(ata->devices.device0.file = strdup(val.str_val))) {
    fprintf(stderr, "Peripheral ATA: Run out of memory\n");
    exit(-1);
  }
}

void ata_dev_size0(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device0.size = val.int_val;
}

void ata_dev_packet0(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device0.packet = val.int_val;
}

void ata_dev_type1(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device1.packet = val.int_val;
}

void ata_dev_file1(union param_val val, void *dat)
{
  ata_host *ata = dat;
  if(!(ata->devices.device1.file = strdup(val.str_val))) {
    fprintf(stderr, "Peripheral ATA: Run out of memory\n");
    exit(-1);
  }
}

void ata_dev_size1(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device1.size = val.int_val;
}

void ata_dev_packet1(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->devices.device1.packet = val.int_val;
}

void ata_enabled(union param_val val, void *dat)
{
  ata_host *ata = dat;
  ata->enabled = val.int_val;
}

void *ata_sec_start(void)
{
  ata_host *new = malloc(sizeof(ata_host));

  if(!new) {
    fprintf(stderr, "Peripheral ATA: Run out of memory\n");
    exit(-1);
  }

  memset(new, 0, sizeof(ata_host));
  new->enabled = 1;
  return new;
}

void ata_sec_end(void *dat)
{
  ata_host *ata = dat;
  struct mem_ops ops;

  if(!ata->enabled) {
    free(dat);
    return;
  }

  /* Connect ata_devices.                                            */
  ata_devices_init(&ata->devices);

  memset(&ops, 0, sizeof(struct mem_ops));

  ops.readfunc32 = ata_read32;
  ops.read_dat32 = dat;
  ops.writefunc32 = ata_write32;
  ops.write_dat32 = dat;

  /* Delays will be readjusted later */
  ops.delayr = 2;
  ops.delayw = 2;

  ata->mem = reg_mem_area(ata->baseaddr, ATA_ADDR_SPACE, 0, &ops);

  reg_sim_reset(ata_reset, dat);
  reg_sim_stat(ata_status, dat);
}

void reg_ata_sec(void)
{
  struct config_section *sec = reg_config_sec("ata", ata_sec_start, ata_sec_end);

  reg_config_param(sec, "enabled", paramt_int, ata_enabled);
  reg_config_param(sec, "baseaddr", paramt_addr, ata_baseaddr);
  reg_config_param(sec, "irq", paramt_int, ata_irq);
  reg_config_param(sec, "dev_type0", paramt_int, ata_dev_type0);
  reg_config_param(sec, "dev_file0", paramt_str, ata_dev_file0);
  reg_config_param(sec, "dev_size0", paramt_int, ata_dev_size0);
  reg_config_param(sec, "dev_packet0", paramt_int, ata_dev_packet0);
  reg_config_param(sec, "dev_type1", paramt_int, ata_dev_type1);
  reg_config_param(sec, "dev_file1", paramt_str, ata_dev_file1);
  reg_config_param(sec, "dev_size1", paramt_int, ata_dev_size1);
  reg_config_param(sec, "dev_packet1", paramt_int, ata_dev_packet1);
}
