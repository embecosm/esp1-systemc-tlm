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
 * Simple hello world for Or1ksim
 *
 * $Id$
 *
 */


#define NOP_NOP         0x0000      /* Normal nop instruction */
#define NOP_EXIT        0x0001      /* End of simulation */
#define NOP_REPORT      0x0002      /* Simple report */
#define NOP_PRINTF      0x0003      /* Simprintf instruction */
#define NOP_PUTC        0x0004      /* JPB: Simputc instruction */
#define NOP_CNT_RESET   0x0005	    /* Reset statistics counters */


/* Do this via the simexit, with l.nop 0 */

void  simexit( int  rc )
{
  __asm__ __volatile__ ( "\tl.nop\t%0" : : "K"( NOP_EXIT ));

}	/* simexit() */


/* Do this via the simputc, with l.nop 4 */

void  printChar( int  c )
{
  __asm__ __volatile__ ( "\tl.nop\t%0" : : "K"( NOP_PUTC ));

}	/* printChar() */


/* Only plain strings for now */

void  printStr( char *str )
{
  int  i;

  for( i = 0; str[i] != '\0' ; i++ ) {
    printChar( (int)(str[i]) );
  }

}	/* printStr() */


main()
{
  printStr( "Hello World\n" );
  simexit( 0 );

}
