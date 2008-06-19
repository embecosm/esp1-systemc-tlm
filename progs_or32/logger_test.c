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

struct  testdev
{
  volatile unsigned char       byte;
  volatile unsigned short int  halfword;
  volatile unsigned long  int  fullword;
};

main()
{
  struct testdev *dev = (struct testdev *)BASEADDR;

  unsigned char       byteRes;
  unsigned short int  halfwordRes;
  unsigned long int   fullwordRes;

  /* Write different sizes */

  simputs( "Writing byte 0xa5 to address 0x" );
  simputh( (unsigned long int)(&(dev->byte)) );
  simputs( "\n" );
  dev->byte     =       0xa5;

  simputs( "Writing half word 0xbeef to address 0x" );
  simputh( (unsigned long int)(&(dev->halfword)) );
  simputs( "\n" );
  dev->halfword =     0xbeef;

  simputs( "Writing full word 0xdeadbeef to address 0x" );
  simputh( (unsigned long int)(&(dev->fullword)) );
  simputs( "\n" );
  dev->fullword = 0xdeadbeef;

  /* Read different sizes */

  byteRes = dev->byte;
  simputs( "Read byte 0x" );
  simputh(  byteRes );
  simputs( " from address 0x" );
  simputh( (unsigned long int)(&(dev->byte)) );
  simputs( "\n" );

  halfwordRes = dev->halfword;
  simputs( "Read half word 0x" );
  simputh( halfwordRes );
  simputs( " from address 0x" );
  simputh( (unsigned long int)(&(dev->halfword)) );
  simputs( "\n" );

  fullwordRes = dev->fullword;
  simputs( "Read full word 0x" );
  simputh( fullwordRes );
  simputs( " from address 0x" );
  simputh( (unsigned long int)(&(dev->fullword)) );
  simputs( "\n" );

  // Terminate the simulation

  simexit( 0 );

}	// main()
