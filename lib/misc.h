/* misc.c -- defs for interface to misc.c
   Copyright (C) 1986, 1995, 1996 Free Software Foundation, Inc.
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#ifndef _misc_h_
#define _misc_h_

#if HAVE_BASENAME
char *basename ();
#else
char *basename __P((char const *path));
#endif

#if HAVE_DIRNAME
char *dirname ();
#else
char *dirname __P((char const *path));
#endif

int tree8_count_levels __P((unsigned int cardinality));
int gets_past_00 __P((char *tok, FILE *input_FILE));
int skip_past_00 __P((FILE *input_FILE));

#endif /* not _misc_h_ */
