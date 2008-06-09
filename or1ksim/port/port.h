#ifndef __STRNDUP__
#define __STRNDUP__

#include <stdlib.h>

#if !defined(HAVE_STRNDUP)
char * strndup (const char *s, size_t n);
#endif

#endif

#ifndef HAVE_ISBLANK
int isblank(int c);
#endif

#ifndef PRIx32
# if SIZEOF_INT == 4
#  define PRIx32 "x"
# elif SIZEOF_LONG == 4
#  define PRIx32 "lx"
# endif
#endif

#ifndef PRIx16
# if SIZEOF_SHORT == 2
#  define PRIx16 "hx"
# else
#  define PRIx16 "x"
# endif
#endif

#ifndef PRIx8
# if SIZEOF_CHAR == 1
#  define PRIx8 "hhx"
# else
#  define PRIx8 "x"
# endif
#endif

#ifndef PRIi32
# if SIZEOF_INT == 4
#  define PRIi32 "i"
# elif SIZEOF_LONG == 4
#  define PRIi32 "li"
# endif
#endif

#ifndef PRId32
# if SIZEOF_INT == 4
#  define PRId32 "d"
# elif SIZEOF_LONG == 4
#  define PRId32 "ld"
# endif
#endif

#ifndef UINT32_C
# if SIZEOF_INT == 4
#  define UINT32_C(c) c
# elif SIZEOF_LONG == 4
#  define UINT32_C(c) c l
# endif
#endif

#ifndef HAVE_UINT32_T
# if SIZEOF_INT == 4
typedef unsigned int uint32_t;
# elif SIZEOF_LONG == 4
typedef unsigned long uint32_t;
# else
#  error "Can't find a 32-bit type"
# endif
#endif

#ifndef HAVE_INT32_T
# if SIZEOF_INT == 4
typedef signed int int32_t;
# elif SIZEOF_LONG == 4
typedef signed long int32_t;
# endif
#endif

#ifndef HAVE_UINT16_T
# if SIZEOF_SHORT == 2
typedef unsigned short uint16_t;
# else
#  error "Can't find a 16-bit type"
# endif
#endif

#ifndef HAVE_INT16_T
# if SIZEOF_SHORT == 2
typedef signed short int16_t;
# endif
#endif

#ifndef HAVE_UINT8_T
# if SIZEOF_CHAR == 1
typedef unsigned char uint8_t;
# else
#  error "Can't find a 8-bit type"
# endif
#endif

#ifndef HAVE_INT8_T
# if SIZEOF_CHAR == 1
typedef signed char int8_t;
# endif
#endif
