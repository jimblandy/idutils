#ifndef _xmalloc_h_
#define _xmalloc_h_ 1

#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
void *xmalloc __P((size_t n));
void *xrealloc __P((void *p, size_t n));

#endif /* _xmalloc_h_ */
