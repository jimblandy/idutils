/* fid.c -- list all tokens in the given file(s)
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <config.h>
#include "system.h"
#include "idfile.h"
#include "bitops.h"
#include "filenames.h"
#include "misc.h"
#include "strxtra.h"
#include "alloc.h"
#include "token.h"
#include "error.h"
#include "pathmax.h"

int get_file_index __P((char *file_name));
int is_hit __P((unsigned char const *hits, int file_number));
int is_hit_1 __P((unsigned char const **hits, int level, int file_number));
void skip_hits __P((unsigned char const **hits, int level));

struct idhead idh;
int tree8_levels;

/* The name this program was run with. */

char const *program_name;

/* If nonzero, display usage information and exit.  */

static int show_help;

/* If nonzero, print the version on standard output then exit.  */

static int show_version;

/* The file name of the ID database.  */

char const *id_file_name;

struct file_link *cw_dlink;
struct file_link **members_0;
unsigned int bits_vec_size;
char *hits_buf;

static struct option const long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { 0 }
};

void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (1);
}

static void
help_me (void)
{
  printf (_("\
Usage: %s [OPTION] FILENAME [FILENAME2]\n"),
	  program_name);
  printf (_("\
List identifiers that occur in FILENAME, or if FILENAME2 is\n\
also given list the identifiers that occur in both files.\n\
\n\
  -f, --file=FILE     file name of ID database\n\
"));
  exit (0);
}

int
main (int argc, char **argv)
{
  int optc;
  int index_1 = -1;
  int index_2 = -1;

  program_name = argv[0];
  for (;;)
    {
      int optc = getopt_long (argc, argv, "f:",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'f':
	  id_file_name = optarg;
	  break;

	default:
	  usage ();
	}
    }

  if (show_version)
    {
      printf ("%s - %s\n", program_name, PACKAGE_VERSION);
      exit (0);
    }

  if (show_help)
    help_me ();

  /* Look for the ID database up the tree */
  id_file_name = look_up (id_file_name);
  if (id_file_name == 0)
    error (1, errno, _("can't locate `ID'"));
  
  init_idh_obstacks (&idh);
  init_idh_tables (&idh);

  cw_dlink = get_current_dir_link ();

  /* Determine absolute name of the directory name to which database
     constituent files are relative. */
  members_0 = read_id_file (id_file_name, &idh);
  bits_vec_size = (idh.idh_files + 7) / 4; /* more than enough */
  tree8_levels = tree8_count_levels (idh.idh_files);

  argc -= optind;
  argv += optind;
  if (argc < 1)
    {
      error (0, 0, _("no file name arguments"));
      usage ();
    }
  if (argc > 2)
    {
      error (0, 0, _("too many file name arguments"));
      usage ();
    }

  index_1 = get_file_index ((argc--, *argv++));
  if (argc)
    index_2 = get_file_index ((argc--, *argv++));

  if (index_1 < 0)
    return 1;

  hits_buf = xmalloc (idh.idh_buf_size);
  fseek (idh.idh_FILE, idh.idh_tokens_offset, SEEK_SET);
  {
    int i;
    for (i = 0; i < idh.idh_tokens; i++)
      {
	unsigned char const *hits;
	
	gets_past_00 (hits_buf, idh.idh_FILE);
	hits = tok_hits_addr (hits_buf);
	if (is_hit (hits, index_1) && (index_2 < 0 || is_hit (hits, index_2)))
	  printf ("%s\n", tok_string (hits_buf));
      }
  }

  return 0;
}

int
get_file_index (char *file_name)
{
  struct file_link **members;
  struct file_link **end = &members_0[idh.idh_files];
  struct file_link *fn_flink = 0;
  int has_slash = (strchr (file_name, '/') != 0);
  int file_name_length = strlen (file_name);
  int index = -1;

  if (strstr (file_name, "./"))
    fn_flink = parse_file_name (file_name, cw_dlink);
    
  for (members = members_0; members < end; members++)
    {
      struct file_link *flink = *members;
      if (fn_flink)
	{
	  if (fn_flink != flink)
	    continue;
	}
      else if (has_slash)
	{
	  char buf[PATH_MAX];
	  int member_length;
	  maybe_relative_path (buf, flink, cw_dlink);
	  member_length = strlen (buf);
	  if (file_name_length > member_length
	      || !strequ (&buf[member_length - file_name_length], file_name))
	    continue;
	}
      else if (!strequ (flink->fl_name, file_name))
	continue;
      if (index >= 0)
	{
	  error (0, 0, _("`%s' is ambiguous"), file_name);
	  return;
	}
      index = members - members_0;
    }
  if (index < 0)
    error (0, 0, _("`%s' not found"), file_name);
  return index;
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
