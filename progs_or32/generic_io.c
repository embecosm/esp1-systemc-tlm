/* ----------------------------------------------------------------------------
 *
 *                  CONFIDENTIAL AND PROPRIETARY INFORMATION
 *                  ========================================
 *
 * Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.
 *
 * This file contains confidential and proprietary information of Embecosm and
 * is protected by copyright, trade secret and other regional, national and
 * international laws, and may be embodied in patents issued or pending.
 *
 * Receipt or possession of this file does not convey any rights to use,
 * reproduce, disclose its contents, or to manufacture, or sell anything it may
 * describe.
 *
 * Reproduction, disclosure or use without specific written authorization of
 * Embecosm is strictly forbidden.
 *
 * Reverse engineering is prohibited.
 *
 * ----------------------------------------------------------------------------
 *
 * Simple test of the generic I/O device.
 *
 * $Id$
 *
 */

#include "utils.h"

#define BASEADDR  0x90000000

struct testdev
{
  volatile unsigned char       byte;
  volatile unsigned short int  halfword;
  volatile unsigned long  int  fullword;
};

main()
{
  volatile struct testdev *dev = (struct testdev *)BASEADDR;

  unsigned char       byteRes;
  unsigned short int  halfwordRes;
  unsigned long int   fullwordRes;

  /* Write different sizes */

  simputs( "Writing byte to address\n" );
  dev->byte     =       0xa5;
  simputs( "Writing half word to address\n" );
  dev->halfword =     0xbeef;
  simputs( "Writing full word to address\n" );
  dev->fullword = 0xdeadbeef;

  /* Read different sizes */

  simputs( "Reading byte from address\n" );
  byteRes = dev->byte;
  simputs( "Reading half word from address\n" );
  halfwordRes = dev->halfword;
  simputs( "Reading full word from address\n" );
  fullwordRes = dev->fullword;

  /* Just so we can see something happened */

  simputs( "I/O All Done\n" );
  simexit( 0 );

}
