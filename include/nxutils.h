#ifndef __UTILS_H
#define __UTILS_H

#include <stdlib.h>
#include <string.h>

#ifndef offsetof
# define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#ifndef container_of
# define container_of(ptr, type, member) \
	(type *)((char *)(ptr) - offsetof(type, member))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif

static inline int
StringStartWith(const char* str, const char* prefix)
{
	return !strncmp(str, prefix, strlen(prefix));
}

#endif
