/* fnprint.c -- print a list of file names
   Copyright (C) 1986, 1995-1996, 2007-2010 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <alloca.h>
#include <unistd.h>
#include <string.h>
#include <pathmax.h>
#include <error.h>

#include "idfile.h"
#include "xnls.h"
#include "iduglobal.h"

static char const *root_name (char const *path);
static char const *suff_name (char const *path);
static int common_prefix_suffix (struct file_link const *flink_1,
				 struct file_link const *flink_2);

extern void usage (void);
static struct file_link *cw_dlink;

/* Return the file name with the suffix stripped off.  */

static char const *
root_name (char const *file_name)
{
  static char file_name_buffer[BUFSIZ];
  char const *dot = strrchr (file_name, '.');

  if (dot)
    {
      int length = dot - file_name;
      strncpy (file_name_buffer, file_name, length);
      file_name_buffer[length] = '\0';
    }
  else
    strcpy (file_name_buffer, file_name);
  return file_name_buffer;
}

/* Return the suffix, including the dot, or an empty string if there
   is no suffix.  */

static char const *
suff_name (char const *file_name)
{
  char const *dot = strrchr (file_name, '.');
  return dot ? dot : "";
}

/* common_prefix_suffix returns non-zero if two file names have a
   fully common directory prefix and a common suffix (i.e., they're
   eligible for coalescing with brace notation.  */

static int
common_prefix_suffix (struct file_link const *flink_1, struct file_link const *flink_2)
{
  return (flink_1->fl_parent == flink_2->fl_parent
	  && strequ (suff_name (flink_1->fl_name), suff_name (flink_2->fl_name)));
}

void
print_filenames (struct file_link **flinkv, enum separator_style separator_style)
{
  struct file_link const *arg;
  struct file_link const *dlink;
  int brace_is_open = 0;
  char *file_name = alloca (sizeof(char) * PATH_MAX);

  while (*flinkv)
    {
      arg = *flinkv++;
      if (*flinkv && (separator_style == ss_braces)
	  && common_prefix_suffix (arg, *flinkv))
	{
	  if (brace_is_open)
	    printf (",%s", root_name (arg->fl_name));
	  else
	    {
	      dlink = arg->fl_parent;
	      if (dlink && dlink != cw_dlink)
		{
		  maybe_relative_file_name (file_name, dlink, cw_dlink);
		  fputs (file_name, stdout);
		  putchar ('/');
		}
	      printf ("{%s", root_name (arg->fl_name));
	    }
	  brace_is_open = 1;
	}
      else
	{
	  if (brace_is_open)
	    printf (",%s}%s", root_name (arg->fl_name), suff_name (arg->fl_name));
	  else
	    {
	      maybe_relative_file_name (file_name, arg, cw_dlink);
	      fputs (file_name, stdout);
	    }
	  brace_is_open = 0;
	  if (*flinkv)
	    {
	      if (separator_style == ss_newline)
		putchar ('\n');
	      else
		putchar (' ');
	    }
	}
    }
  putchar ('\n');
}

enum separator_style
parse_separator_style (char const *arg)
{
  MAYBE_RETURN_PREFIX_MATCH (arg, "braces", ss_braces);
  MAYBE_RETURN_PREFIX_MATCH (arg, "space", ss_space);
  MAYBE_RETURN_PREFIX_MATCH (arg, "newline", ss_newline);
  error (0, 0, _("invalid `--separator' style: `%s'"), arg);
  usage ();
  return ss_bogus;
}
