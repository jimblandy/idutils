/* idx.c -- simple interface for testing scanners scanners
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
#include <getopt.h>

#include <config.h>
#include "alloc.h"
#include "system.h"
#include "misc.h"
#include "filenames.h"
#include "scanners.h"
#include "idfile.h"
#include "pathmax.h"

void scan_files __P((struct idhead *idhp));
void scan_member_file __P((struct member_file const *member));

char const *program_name;
char *lang_map_file_name = 0;
int show_version = 0;
int show_help = 0;
struct idhead idh;

void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (1);
}

static struct option const long_options[] =
{
  { "lang-map", required_argument, 0, 'm' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { 0 }
};

static void
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... [FILE]...\n"),
	  program_name);

  printf (_("\
Print all tokens found in a source file.\n\
  -m, --lang-map=FILE     use FILE to file names onto source language\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
\n\
The following arguments apply to the language-specific scanners:\n\
"));
  language_help_me ();
  exit (0);
}

int
main (int argc, char **argv)
{
  char *arg;

  program_name = argv[0];
  for (;;)
    {
      int optc = getopt_long (argc, argv, "o:f:i:x:l:m:uvs",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'm':
	  lang_map_file_name = optarg;
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

  argc -= optind;
  argv += optind;

  init_idh_obstacks (&idh);
  init_idh_tables (&idh);
  parse_language_map (lang_map_file_name);
  {
    struct file_link *cwd_link = get_current_dir_link ();
    while (argc--)
      walk_flink (parse_file_name (*argv++, cwd_link), 0);
    mark_member_file_links (&idh);
    obstack_init (&tokens_obstack);
    scan_files (&idh);
  }

  return 0;
}

void
scan_files (struct idhead *idhp)
{
  struct member_file **members_0
    = (struct member_file **) hash_dump (&idhp->idh_member_file_table,
					 0, member_file_qsort_compare);
  struct member_file **end = &members_0[idhp->idh_member_file_table.ht_fill];
  struct member_file **members;

  for (members = members_0; members < end; members++)
    scan_member_file (*members);
  free (members_0);
}

void
scan_member_file (struct member_file const *member)
{
  struct lang_args const *lang_args = member->mf_lang_args;
  struct language const *lang = lang_args->la_language;
  get_token_func_t get_token = lang->lg_get_token;
  struct file_link *flink = member->mf_link;
  FILE *source_FILE;
  size_t bytes;

  chdir_to_link (flink->fl_parent);
  source_FILE = open_source_FILE (flink->fl_name);
  if (source_FILE)
    {
      void const *args = lang_args->la_args_digested;
      int flags;
      struct token *token;

      while ((token = (*get_token) (source_FILE, args, &flags)) != NULL)
	{
	  puts (TOKEN_NAME (token));
	  obstack_free (&tokens_obstack, token);
	}
      close_source_FILE (source_FILE);
    }
}
