/* misc.c -- miscellaneous common functions
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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "strxtra.h"
#include "misc.h"

char const *
basename (char const *path)
{
  char *base;

  base = strrchr (path, '/');
  if (base)
    return ++base;
  else
    return path;
}

char const *
dirname (char const *path)
{
  char *base;

  base = strrchr (path, '/');
  if (base)
    return strndup (path, base - path);
  else
    return ".";
}

/* This is like fgets(3s), except that lines are delimited by NULs
   rather than newlines.  Also, we return the number of characters
   gotten rather than the address of buf0.  */
int
fgets0 (char *buf0, int size, FILE * in_FILE)
{
  char *buf;
  int c;
  char *end;

  buf = buf0;
  end = &buf[size];
  while ((c = getc (in_FILE)) > 0 && buf < end)
    *buf++ = c;
  *buf = '\0';
  return (buf - buf0);
}

extern char const *program_name;

void
filerr (char const *syscall, char const *file_name)
{
  fprintf (stderr, "%s: Cannot %s `%s' (%s)\n", program_name, syscall, file_name, strerror (errno));
}

int
tree8_count_levels (unsigned int cardinality)
{
  int levels = 1;
  cardinality--;
  while (cardinality >>= 3)
    ++levels;
  return levels;
}

int
gets_past_00 (char *tok, FILE *input_FILE)
{
  int got = 0;
  int c;
  do
    {
      do
	{
	  got++;
	  c = getc (input_FILE);
	  *tok++ = c;
	}
      while (c > 0);
      got++;
      c = getc (input_FILE);
      *tok++ = c;
    }
  while (c > 0);
  return got - 2;
}

int
skip_past_00 (FILE *input_FILE)
{
  int skipped = 0;
  do
    {
      do
	skipped++;
      while (getc (input_FILE) > 0);
      skipped++;
    }
  while (getc (input_FILE) > 0);
  return skipped;
}
