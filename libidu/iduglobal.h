#ifndef _iduglobal_h
#define _iduglobal_h

/* iduglobal.h -- global definitions for libidu
   Copyright (C) 1995, 1999, 2005, 2007, 2009-2010 Free Software Foundation,
   Inc.
   Written by Claudio Fontana <sick_soul@users.sourceforge.net>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef cardinalityof
#define cardinalityof(ARRAY) (sizeof (ARRAY) / sizeof ((ARRAY)[0]))
#endif

#ifndef strequ
#define strequ(s1, s2)          (strcmp ((s1), (s2)) == 0)
#define strnequ(s1, s2, n)      (strncmp ((s1), (s2), (n)) == 0)
#endif

#ifndef obstack_chunk_alloc
#define obstack_chunk_alloc xmalloc
#endif
#ifndef obstack_chunk_free
#define obstack_chunk_free free
#endif

#define CLONE(o, t, n) ((t *) memcpy (xmalloc (sizeof(t) * (n)), (o), sizeof (t) * (n)))

#define DEBUG(args) /* printf args */

#if HAVE_LINK
#define MAYBE_FNM_CASEFOLD 0
#else
#define MAYBE_FNM_CASEFOLD FNM_CASEFOLD
#endif

#if HAVE_LINK
#define IS_ABSOLUTE(_dir_) ((_dir_)[0] == '/')
#define SLASH_STRING "/"
#define SLASH_CHAR '/'
#define DOT_DOT_SLASH "../"
#else
/* NEEDSWORK: prefer forward-slashes as a user-configurable option.  */
#define IS_ABSOLUTE(_dir_) ((_dir_)[1] == ':')
#define SLASH_STRING "\\/"
#define SLASH_CHAR '\\'
#define DOT_DOT_SLASH "..\\"
#endif

#define STREQ(a, b) (strcmp (a, b) == 0)

static inline char *
bad_cast (char const *s)
{
  return (char *) s;
}

#endif /* _iduglobal_h */
