/* filenames.h -- defs for interface to filenames.c
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

#ifndef _filenames_h_
#define _filenames_h_

char const *relative_file_name __P((char const *dir, char const *arg));
char const *span_file_name __P((char const *dir, char const *arg));
char const *root_name __P((char const *path));
char const *suff_name __P((char const *path));
int can_crunch __P((char const *path1, char const *path2));
char const *look_up __P((char const *arg));
void cannoname __P((char *n));
char const *kshgetwd __P((char *pathname));
char const *unsymlink __P((char *n));
FILE *open_source_FILE __P((char *file_name, char const *filter));
void close_source_FILE __P((FILE *fp, char const *filter));
char const *get_sccs __P((char const *dir, char const *base, char const *sccs_dir));
char const *co_rcs __P((char const *dir, char const *base, char const *rcs_dir));

#endif /* not _filenames_h_ */
