/* static char copyright[] = "@(#)Copyright (c) 1986, Greg McGary";
   static char sccsid[] = "@(#)idx.c	1.2 86/10/17"; */

#include <stdio.h>
#include <string.h>

#include <config.h>
#include "misc.h"
#include "filenames.h"
#include "scanners.h"

void idxtract __P((char *path));

char const *program_name;

static void
usage (void)
{
  fprintf (stderr, "Usage: %s [-u] [+/-a<ccc>] [-c<ccc>] files\n", program_name);
  exit (1);
}

int
main (int argc, char **argv)
{
  char *arg;
  int op;

  program_name = basename ((argc--, *argv++));

  init_scanners ();

  while (argc)
    {
      arg = (argc--, *argv++);
      switch (op = *arg++)
	{
	case '-':
	case '+':
	  break;
	default:
	  (argc++, --argv);
	  goto argsdone;
	}
      switch (*arg++)
	{
	case 'S':
	  set_scan_args (op, arg);
	  break;
	default:
	  usage ();
	}
    }
argsdone:

  if (argc == 0)
    usage ();

  while (argc)
    idxtract ((argc--, *argv++));

  return 0;
}

void
idxtract (char *file_name)
{
  char const *key;
  FILE *source_FILE;
  int flags;
  char const *suffix;
  char const *filter;
  char const *lang_name;
  char const *(*scanner) (FILE*, int*);

  suffix = strrchr (file_name, '.');
  lang_name = get_lang_name (suffix);
  scanner = get_scanner (lang_name);
  if (scanner == NULL)
    return;
  source_FILE = open_source_FILE (file_name, filter = get_filter (suffix));
  if (source_FILE == NULL)
    return;

  while ((key = (*scanner) (source_FILE, &flags)) != NULL)
    puts (key);

  close_source_FILE (source_FILE, filter);
}
