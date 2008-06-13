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


/* l.nop constants only used here */

#define NOP_NOP         0x0000      /* Normal nop instruction */
#define NOP_EXIT        0x0001      /* End of simulation */
#define NOP_REPORT      0x0002      /* Simple report */
#define NOP_PRINTF      0x0003      /* Simprintf instruction */
#define NOP_PUTC        0x0004      /* JPB: Simputc instruction */
#define NOP_CNT_RESET   0x0005	    /* Reset statistics counters */


void  simexit( int  rc )
{
  __asm__ __volatile__ ( "\tl.nop\t%0" : : "K"( NOP_EXIT ));

}	/* simexit() */


void  simputc( int  c )
{
  __asm__ __volatile__ ( "\tl.nop\t%0" : : "K"( NOP_PUTC ));

}	/* simputc() */


extern void  simputh( int  i )
{
  int   lsd = i & 0xf;
  char  ch  = lsd < 10 ? '0' + lsd : 'A' + lsd - 10;

  if( i > 0 ) {
    simputh( i >> 4 );
  }

  simputc( ch );

}	/* simputh() */
  
    
void  simputs( char *str )
{
  int  i;

  for( i = 0; str[i] != '\0' ; i++ ) {
    simputc( (int)(str[i]) );
  }

}	/* simputs() */
