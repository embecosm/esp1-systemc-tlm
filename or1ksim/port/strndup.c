
#include "config.h"
#include "port.h"

#include <string.h>

#if !defined(HAVE_STRNDUP)

/* Taken from glibc */
char *
strndup (const char *s, size_t n)
{
	char *new;
	size_t len = strlen (s);
	if (len>n)
		len=n;

	new = (char *) malloc (len + 1);

	if (new == NULL)
		return NULL;

	new[len] = '\0';
	return (char *) memcpy (new, s, len);
}

#endif
