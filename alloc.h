/* alloc.h -- convenient interface macros for malloc(3) & friends
   Copyright (C) 1986, 1995 Greg McGary
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _alloc_h_
#define _alloc_h_

#if HAVE_STDLIB_H
#include <stdlib.h>
#else /* not HAVE_STDLIB_H */
#if HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#endif /* not HAVE_STDLIB_H */

#define	CALLOC(type, n)	((type *) calloc (sizeof (type), (n)))
#define MALLOC(type, n) ((type *) malloc (sizeof (type) * (n)))
#define REALLOC(old, type, n) ((type *) realloc ((old), sizeof (type) * (n)))

#endif /* not _alloc_h_ */
