/* strxtra.c -- convenient string manipulation macros
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

#ifndef _strxtra_h_
#define _strxtra_h_

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#define strequ(s1, s2)		(strcmp((s1), (s2)) == 0)
#define strnequ(s1, s2, n)	(strncmp((s1), (s2), (n)) == 0)
#define strcaseequ(s1, s2)	(strcasecmp((s1), (s2)) == 0)
#define strncaseequ(s1, s2, n)	(strncasecmp((s1), (s2), (n)) == 0)
#ifndef HAVE_STRDUP
#define strdup(s)		(strcpy(calloc(1, strlen(s)+1), (s)))
#endif
#define strndup(s, n)		(strncpy(calloc(1, (n)+1), (s), (n)))

#endif /* not _strxtra_h_ */
