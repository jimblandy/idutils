/* fid.c -- list all tokens in the given file(s)
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <config.h>
#include "idfile.h"
#include "idarg.h"
#include "bitops.h"
#include "filenames.h"
#include "misc.h"
#include "strxtra.h"
#include "alloc.h"
#include "token.h"

int get_idarg_index __P((char const *file_name));
int is_hit __P((unsigned char const *hits, int file_number));
int is_hit_1 __P((unsigned char const **hits, int level, int file_number));
void skip_hits __P((unsigned char const **hits, int level));

FILE *id_FILE;
struct idhead idh;
struct idarg *idarg_0;
int tree8_levels;
char const *program_name;

static void
usage (void)
{
  fprintf (stderr, "Usage: %s [-f<file>] file1 file2\n", program_name);
  exit (1);
}

int
main (int argc, char **argv)
{
  char const *id_file = IDFILE;
  char *buf;
  int op;
  int i;
  int index_1 = -1;
  int index_2 = -1;

  program_name = basename ((argc--, *argv++));

  while (argc)
    {
      char const *arg = (argc--, *argv++);
      switch (op = *arg++)
	{
	case '-':
	case '+':
	  break;
	default:
	  (argc++, --argv);
	  goto argsdone;
	}
      while (*arg)
	switch (*arg++)
	  {
	  case 'f':
	    id_file = arg;
	    goto nextarg;
	  default:
	    usage ();
	  }
  nextarg:;
    }
argsdone:

  id_file = look_up (id_file);
  if (id_file == NULL)
    {
      filerr ("open", id_file);
      return 1;
    }
  id_FILE = init_idfile (id_file, &idh, &idarg_0);
  if (id_FILE == NULL)
    {
      filerr ("open", id_file);
      return 1;
    }
  switch (argc)
    {
    case 2:
      index_2 = get_idarg_index (argv[1]);
      /* fall through */
    case 1:
      index_1 = get_idarg_index (argv[0]);
      break;
    default:
      usage ();
    }

  if (index_1 < 0)
    return 1;

  buf = MALLOC (char, idh.idh_buf_size);
  fseek (id_FILE, idh.idh_tokens_offset, 0);
  tree8_levels = tree8_count_levels (idh.idh_paths);

  for (i = 0; i < idh.idh_tokens; i++)
    {
      unsigned char const *hits;

      gets_past_00 (buf, id_FILE);
      hits = tok_hits_addr (buf);
      if (is_hit (hits, index_1) && (index_2 < 0 || is_hit (hits, index_2)))
	printf ("%s\n", tok_string (buf));
    }

  return 0;
}

int
get_idarg_index (char const *file_name)
{
  struct idarg *idarg;
  int file_name_length = strlen (file_name);
  struct idarg *end = &idarg_0[idh.idh_paths];

  for (idarg = idarg_0; idarg < end; ++idarg)
    {
      int arg_length = strlen (idarg->ida_arg);
      int prefix_length = arg_length - file_name_length;
      if (prefix_length < 0
	  || (prefix_length > 0 && idarg->ida_arg[prefix_length - 1] != '/'))
	continue;
      if (strequ (&idarg->ida_arg[prefix_length], file_name))
	return idarg->ida_index;
    }
  fprintf (stderr, "%s: not found\n", file_name);
  return -1;
}

int
is_hit (unsigned char const *hits, int file_number)
{
  return is_hit_1 (&hits, tree8_levels, file_number);
}

int
is_hit_1 (unsigned char const **hits, int level, int file_number)
{
  int file_hit = 1 << ((file_number >> (3 * --level)) & 7);
  int hit = *(*hits)++;
  int bit;
  
  if (!(file_hit & hit))
    return 0;
  if (level == 0)
    return 1;
  
  for (bit = 1; (bit < file_hit) && (bit & 0xff); bit <<= 1)
    {
      if (hit & bit)
	skip_hits (hits, level);
    }
  return is_hit_1 (hits, level, file_number);
}

void
skip_hits (unsigned char const **hits, int level)
{
  int hit = *(*hits)++;
  int bit;
  
  if (--level == 0)
    return;
  for (bit = 1; bit & 0xff; bit <<= 1)
    {
      if (hit & bit)
	skip_hits (hits, level);
    }
}
