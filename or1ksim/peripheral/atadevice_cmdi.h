/*
    atadevice_cmdi.h -- ATA Device command interpreter
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


/**********************************************************************/
/* define FIRMWARE revision; current revision of OR1Ksim ATA code.    */
/* coded using the release date (yyyymmdd, but shuffled)              */
#define FIRMWARE "02207031"


/**********************************************************************/
/* Define default CHS translation parameters                          */
/* bytes per sector                                                   */
#define BYTES_PER_SECTOR 512

/* number of default heads ( -1)                                      */
#define HEADS 7

/* number of default sectors per track                                */
#define SECTORS 32


/**********************************************************************/
/* Define supported DMA and PIO modes                                 */
/* define the highest Multiword DMA mode;  2, 1, 0, -1(no DMA)        */
#define MWDMA 2

/* define the highest supported PIO mode; 4, 3, 2, 1, 0(obsolete)     */
#define PIO 4


/**********************************************************************/
/* Define DMA and PIO cycle times                                     */
/* define the minimum Multiword DMA cycle time per word (in nsec)     */
#define MIN_MWDMA_CYCLE_TIME 100

/* define the manufacturer's recommended Multiword DMA cycle time     */
#define RECOMMENDED_MWDMA_CYCLE_TIME 100

/* define the minimum PIO cycle time per word (in nsec), no IORDY     */
#define MIN_PIO_CYCLE_TIME_NO_IORDY 100

/* define the minimum PIO cycle time per word (in nsec), with IORDY   */
#define MIN_PIO_CYCLE_TIME_IORDY 100


/**********************************************************************/
/* Define supported command sets                                      */
/* 1=command is supported                                             */
/* 0=command is not (yet) supported                                   */
#define SUPPORT_NOP_CMD                      0
#define SUPPORT_READ_BUFFER_CMD              0
#define SUPPORT_WRITE_BUFFER_CMD             0
#define SUPPORT_HOST_PROTECTED_AREA          0
#define SUPPORT_DEVICE_RESET_CMD             1
#define SUPPORT_SERVICE_INTERRUPT            0
#define SUPPORT_RELEASE_INTERRUPT            0
#define SUPPORT_LOOKAHEAD                    0
#define SUPPORT_WRITE_CACHE                  0
#define SUPPORT_POWER_MANAGEMENT             0
#define SUPPORT_REMOVABLE_MEDIA              0
#define SUPPORT_SECURITY_MODE                0
#define SUPPORT_SMART                        0
#define SUPPORT_SET_MAX                      0
#define SET_FEATURES_REQUIRED_AFTER_POWER_UP 0
#define SUPPORT_POWER_UP_IN_STANDBY_MODE     0
#define SUPPORT_REMOVABLE_MEDIA_NOTIFICATION 0
#define SUPPORT_APM                          0
#define SUPPORT_CFA                          0
#define SUPPORT_READ_WRITE_DMA_QUEUED        0
#define SUPPORT_DOWNLOAD_MICROCODE           0

/*
Queue depth defines the maximum queue depth supported by the device.
This includes all commands for which the command acceptance has occured
and command completion has not occured. This value is used for the DMA
READ/WRITE QUEUE command.

QUEUE_DEPTH = actual_queue_depth -1
*/
#define QUEUE_DEPTH 0



/**********************************************************************/
/* Internal defines                                                   */
#define PACKET_SIGNATURE 1


/*
   ----------------
   -- Prototypes --
   ----------------
*/
int  ata_device_execute_cmd(ata_device *device);

void ata_device_reset_cmd(ata_device *device);
void ata_execute_device_diagnostics_cmd(ata_device *device);
void ata_identify_device_cmd(ata_device *device);
void ata_initialize_device_parameters_cmd(ata_device *device);
void ata_read_sectors_cmd(ata_device *device);
