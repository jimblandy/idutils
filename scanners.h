/* scanners.h -- defs for interface to scanners.c
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

#ifndef _scanners_h_
#define _scanners_h_

typedef char *(*get_token_t) (FILE*, int*);

char const *get_lang_name (char const *suffix);
char const *get_filter (char const *suffix);
char const *(*get_scanner (char const *lang_name)) (FILE *input_FILE, int *flags);
void set_scan_args (int op, char *arg);
void init_scanners (void);

#endif /* not _scanners_h_ */
