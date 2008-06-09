
#include "config.h"

#include "port.h"

#ifndef HAVE_ISBLANK
/*
 *isblank() is a GNU extension
 *not available on Solaris, for example
 */
int isblank(int c)
{
  return (c==' ') || (c=='\t');
}
#endif

