/* filenames.h -- defs for interface to filenames.c
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

#ifndef _filenames_h_
#define _filenames_h_

#define	ID_FILE_NAME "ID"

char const *relative_file_name __P((char const *dir, char const *arg));
char const *span_file_name __P((char const *dir, char const *arg));
char const *root_name __P((char const *path));
char const *suff_name __P((char const *path));
int can_crunch __P((char const *path1, char const *path2));
char const *look_up __P((char const *arg));
void cannoname __P((char *n));
char const *kshgetwd __P((char *pathname));
char const *unsymlink __P((char *n));
FILE *open_source_FILE __P((char *file_name));
void close_source_FILE __P((FILE *fp));
char const *get_sccs __P((char const *dir, char const *base, char const *sccs_dir));
char const *co_rcs __P((char const *dir, char const *base, char const *rcs_dir));

#endif /* not _filenames_h_ */
