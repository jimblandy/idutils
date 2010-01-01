/* fnid.c -- report which files constitute an ID database
   Copyright (C) 1996, 2008-2010 Free Software Foundation, Inc.
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
#include <fnmatch.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <alloca.h>
#include <xalloc.h>
#include <pathmax.h>
#include <error.h>

#include "closeout.h"
#include "xnls.h"
#include "idfile.h"
#include "iduglobal.h"
#include "progname.h"

static int show_version = 0;
static int show_help = 0;
static struct file_link *cw_dlink;

struct idhead idh;

/* How shall we separate file names?  */

static enum separator_style separator_style = ss_contextual;

void usage (void) __attribute__((__noreturn__));
void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (EXIT_FAILURE);
}

static struct option const long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "separator", required_argument, 0, 'S' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  {NULL, 0, NULL, 0}
};

static void __attribute__((__noreturn__))
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... [PATTERN]...\n\
"), program_name);

  printf (_("\
Print constituent file names that match PATTERN,\n\
using shell-style wildcards.\n\
  -f, --file=FILE        file name of ID database\n\
  -S, --separator=STYLE  STYLE is one of `braces', `space' or `newline'\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
"));
  printf (_("\nReport bugs to " PACKAGE_BUGREPORT "\n\n"));
  exit (EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
  set_program_name (argv[0]);
  idh.idh_file_name = 0;

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
      int optc = getopt_long (argc, argv, "f:S:",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'f':
	  idh.idh_file_name = optarg;
	  break;

	case 'S':
	  separator_style = parse_separator_style (optarg);
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

  if (separator_style == ss_contextual)
    {
      if (isatty (STDOUT_FILENO))
	separator_style = DEFAULT_SEPARATOR_STYLE;
      else
	separator_style = ss_newline;
    }

  argc -= optind;
  argv += optind;
  if (argc == 0)
    {
      static char star[] = "*";
      static char *starp = star;
      argc = 1;
      argv = &starp;
    }

  /* Look for the ID database up the tree */
  idh.idh_file_name = locate_id_file_name (idh.idh_file_name);
  if (idh.idh_file_name == 0)
    error (EXIT_FAILURE, errno, _("can't locate `ID'"));

  init_idh_obstacks (&idh);
  init_idh_tables (&idh);

  cw_dlink = get_current_dir_link ();
  {
    struct file_link **members = read_id_file (idh.idh_file_name, &idh);
    struct file_link **members_N = &members[idh.idh_files];
    struct file_link **flinkv_0 = xmalloc (sizeof(struct file_link *) * (idh.idh_files + 1));
    struct file_link **flinkv = flinkv_0;
    char **patv_0 = xmalloc (sizeof(char *) * (argc * 2));
    char **patv_N;
    char **patv = patv_0;
    char *file_name = alloca (PATH_MAX);

    for ( ; argc; argc--, argv++)
      {
	char *arg = *argv;
	*patv++ = arg;
	if (*arg != '*' && *arg != '/')
	  {
	    char *pat = xmalloc (strlen (arg) + 2);
	    sprintf (pat, "*/%s", arg);
	    *patv++ = pat;
	  }
      }
    patv_N = patv;

    for ( ; members < members_N; members++)
      {
	maybe_relative_file_name (file_name, *members, cw_dlink);
	for (patv = patv_0; patv < patv_N; patv++)
	  {
	    if (fnmatch (*patv, file_name, MAYBE_FNM_CASEFOLD) == 0)
	      {
		*flinkv++ = *members;
		break;
	      }
	  }
      }
    *flinkv = 0;
    print_filenames (flinkv_0, separator_style);
  }
  return 0;
}
