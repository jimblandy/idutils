/* misc.c -- defs for interface to misc.c
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

#ifndef _misc_h_
#define _misc_h_

#if !HAVE_BASENAME
char const *basename (char const *path);
#endif
#if !HAVE_DIRNAME
char const *dirname (char const *path);
#endif
int fgets0 (char *buf0, int size, FILE *in_FILE);
void filerr (char const *syscall, char const *file_name);
int tree8_count_levels (unsigned int cardinality);
int gets_past_00 (char *tok, FILE *input_FILE);
int skip_past_00 (FILE *input_FILE);

#endif /* not _misc_h_ */
