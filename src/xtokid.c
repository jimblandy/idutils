/* idx.c -- simple interface for testing scanners scanners
   Copyright (C) 1986, 1995-1996, 1999, 2007-2010 Free Software Foundation,
   Inc.
   Written by Greg McGary <gkm@gnu.ai.mit.edu>

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

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "argv-iter.h"
#include "closeout.h"
#include "error.h"
#include "pathmax.h"
#include "progname.h"
#include "quote.h"
#include "quotearg.h"
#include "xalloc.h"

#include "xnls.h"
#include "scanners.h"
#include "idfile.h"
#include "iduglobal.h"

static void scan_files (struct idhead *idhp);
static void scan_member_file (struct member_file const *member);
void usage (void) __attribute__((__noreturn__));

static char *lang_map_file_name = 0;
static int show_version = 0;
static int show_help = 0;
struct idhead idh;
static struct file_link *cw_dlink;

void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (EXIT_FAILURE);
}

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  FILES0_FROM_OPTION = CHAR_MAX +1,
};

static struct option const long_options[] =
{
  { "include", required_argument, 0, 'i' },
  { "exclude", required_argument, 0, 'x' },
  { "lang-option", required_argument, 0, 'l' },
  { "lang-map", required_argument, 0, 'm' },
  { "default-lang", required_argument, 0, 'd' },
  { "prune", required_argument, 0, 'p' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { "files0-from", required_argument, NULL, FILES0_FROM_OPTION },
  {NULL, 0, NULL, 0}
};

static void __attribute__((__noreturn__))
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"), program_name);

  printf (_("\
Print all tokens found in a source file.\n\
  -i, --include=LANGS     include languages in LANGS (default: \"C C++ asm\")\n\
  -x, --exclude=LANGS     exclude languages in LANGS\n\
  -l, --lang-option=L:OPT pass OPT as a default for language L (see below)\n\
  -m, --lang-map=MAPFILE  use MAPFILE to map file names onto source language\n\
  -d, --default-lang=LANG  make LANG the default source language\n\
  -p, --prune=NAMES       exclude the named files and/or directories\n\
\n\
      --files0-from=F     tokenize only the files specified by\n\
                           NUL-terminated names in file F\n\
\n\
      --help              display this help and exit\n		\
      --version           output version information and exit\n\
\n\
The following arguments apply to the language-specific scanners:\n\
"));
  language_help_me ();
  printf (_("\nReport bugs to " PACKAGE_BUGREPORT "\n\n"));
  exit (EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
  bool ok;
  int nfiles;
  char *files_from = NULL;

  set_program_name (argv[0]);

#if ENABLE_NLS
  /* Set locale according to user's wishes.  */
  setlocale (LC_ALL, "");

  /* Tell program which translations to use and where to find.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  atexit (close_stdout);

  for (;;)
    {
      int optc = getopt_long (argc, argv, "i:x:l:m:d:p:",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'i':
	  include_languages (optarg);
	  break;

	case 'x':
	  exclude_languages (optarg);
	  break;

	case 'l':
	  language_save_arg (optarg);
	  break;

	case 'm':
	  lang_map_file_name = optarg;
	  break;

	case 'd':
	  set_default_language (optarg);
	  break;

	case 'p':
	  if (cw_dlink == 0)
	    cw_dlink = init_walker (&idh);
	  prune_file_names (optarg, cw_dlink);
	  break;

	case FILES0_FROM_OPTION:
	  files_from = optarg;
	  break;

	default:
	  usage ();
	}
    }

  if (show_version)
    {
      printf ("%s - %s\n", program_name, PACKAGE_VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    help_me ();

  nfiles = argc - optind;

  struct argv_iterator *ai;
  if (files_from)
    {
      /* When using --files0-from=F, you may not specify any files
	 on the command-line.  */
      if (nfiles != 0)
	{
	  error (0, 0, _("extra operand %s"), quote (argv[optind]));
	  fprintf (stderr, "%s\n",
		   _("file operands cannot be combined with --files0-from"));
	  usage();
	}

      if (! (strequ (files_from, "-") || freopen (files_from, "r", stdin)))
	error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
	       quote (files_from));

      ai = argv_iter_init_stream (stdin);
    }
  else
     {
      static char *cwd_only[2];
      cwd_only[0] = bad_cast (".");
      cwd_only[1] = NULL;
      char **files = (optind < argc ? argv + optind : cwd_only);
      ai = argv_iter_init_argv (files);
    }

  language_getopt ();
  if (cw_dlink == 0)
    cw_dlink = init_walker (&idh);
  parse_language_map (lang_map_file_name);

  /* Walk the file and directory names given on the command line.  */
  ok = true;
  while (true)
    {
      bool skip_file = false;
      enum argv_iter_err ai_err;
      char *file_name = argv_iter (ai, &ai_err);
      if (ai_err == AI_ERR_EOF)
        break;
      if (!file_name)
        {
          switch (ai_err)
            {
            case AI_ERR_READ:
              error (0, errno, _("%s: read error"), quote (files_from));
              continue;

            case AI_ERR_MEM:
              xalloc_die ();

            default:
              assert (!"unexpected error code from argv_iter");
            }
        }
      if (files_from && STREQ (files_from, "-") && STREQ (file_name, "-"))
        {
          /* Give a better diagnostic in an unusual case:
             printf - | du --files0-from=- */
          error (0, 0, _("when reading file names from stdin, "
                         "no file name of %s allowed"),
                 quote (file_name));
          skip_file = true;
        }

      /* Report and skip any empty file names.  */
      if (!file_name[0])
        {
          /* Diagnose a zero-length file name.  When it's one
             among many, knowing the record number may help.
             FIXME: currently print the record number only with
             --files0-from=FILE.  Maybe do it for argv, too?  */
          if (files_from == NULL)
            error (0, 0, "%s", _("invalid zero-length file name"));
          else
            {
              /* Using the standard `filename:line-number:' prefix here is
                 not totally appropriate, since NUL is the separator, not NL,
                 but it might be better than nothing.  */
              unsigned long int file_number = argv_iter_n_args (ai);
              error (0, 0, "%s:%lu: %s", quotearg_colon (files_from),
                     file_number, _("invalid zero-length file name"));
            }
          skip_file = true;
        }

      if (skip_file)
        ok = false;
      else
        {
          struct file_link *flink = parse_file_name (file_name, cw_dlink);
          if (flink)
            walk_flink (flink, 0);
	  /* FIXME: walk_flink can fail, so should return status.
	     Then caller can continue with other arguments.  */
        }
    }

  argv_iter_free (ai);
  mark_member_file_links (&idh);
  obstack_init (&tokens_obstack);
  scan_files (&idh);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
scan_files (struct idhead *idhp)
{
  struct member_file **members_0
    = (struct member_file **) hash_dump (&idhp->idh_member_file_table,
					 0, member_file_qsort_compare);
  struct member_file **end = &members_0[idhp->idh_member_file_table.ht_fill];
  struct member_file **members;

  if (largest_member_file > MAX_LARGEST_MEMBER_FILE)
    largest_member_file = MAX_LARGEST_MEMBER_FILE;
  scanner_buffer = xmalloc (largest_member_file + 1);

  for (members = members_0; members < end; members++)
    scan_member_file (*members);

  free (scanner_buffer);
  free (members_0);
}

static void
scan_member_file (struct member_file const *member)
{
  struct lang_args const *lang_args = member->mf_lang_args;
  struct language const *lang = lang_args->la_language;
  get_token_func_t get_token = lang->lg_get_token;
  struct file_link *flink = member->mf_link;
  FILE *source_FILE;

  chdir_to_link (flink->fl_parent);
  source_FILE = fopen (flink->fl_name, "r");
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
      fclose (source_FILE);
    }
  else
    error (0, errno, _("can't open `%s'"), flink->fl_name);
}
