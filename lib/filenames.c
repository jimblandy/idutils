/* filenames.c -- file & directory name manipulations
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include "system.h"
#include "strxtra.h"
#include "filenames.h"
#include "misc.h"
#include "error.h"

extern char *xgetcwd __P((void));

/* root_name returns the base name of the file with any leading
 * directory information or trailing suffix stripped off. Examples:
 *
 *   /usr/include/stdio.h   ->   stdio
 *   fred                   ->   fred
 *   barney.c               ->   barney
 *   bill/bob               ->   bob
 *   /                      ->   < null string >
 */
char const *
root_name (char const *path)
{
  static char file_name_buffer[BUFSIZ];
  char const *root;
  char const *dot;

  root = strrchr (path, '/');
  if (root == NULL)
    root = path;
  else
    root++;

  dot = strrchr (root, '.');
  if (dot == NULL)
    strcpy (file_name_buffer, root);
  else
    {
      strncpy (file_name_buffer, root, dot - root);
      file_name_buffer[dot - root] = '\0';
    }
  return file_name_buffer;
}

/* suff_name returns the suffix (including the dot) or a null string
 * if there is no suffix. Examples:
 *
 *   /usr/include/stdio.h   ->   .h
 *   fred                   ->   < null string >
 *   barney.c               ->   .c
 *   bill/bob               ->   < null string >
 *   /                      ->   < null string >
 */
char const *
suff_name (char const *path)
{
  char const *dot;

  dot = strrchr (path, '.');
  if (dot == NULL)
    return "";
  return dot;
}

int
can_crunch (char const *path1, char const *path2)
{
  char const *slash1;
  char const *slash2;

  slash1 = strrchr (path1, '/');
  slash2 = strrchr (path2, '/');

  if (slash1 == NULL && slash2 == NULL)
    return strequ (suff_name (path1), suff_name (path2));
  if ((slash1 - path1) != (slash2 - path2))
    return 0;
  if (!strnequ (path1, path2, slash1 - path1))
    return 0;
  return strequ (suff_name (slash1), suff_name (slash2));
}

/* look_up adds ../s to the beginning of a file name until it finds
 * the one that really exists. Returns NULL if it gets all the way
 * to / and never finds it.
 *
 * If the file name starts with /, just return it as is.
 *
 * This routine is used to locate the ID database file.
 */
char const *
look_up (char const *arg)
{
  static char file_name_buffer[BUFSIZ];
  char *buf = file_name_buffer;
  char *id_path = 0;
  struct stat rootb;
  struct stat statb;

  if (arg == 0)
    {
      id_path = getenv ("IDPATH");
      if (id_path)
	{
	  id_path = strdup (id_path);
	  arg = strtok (id_path, ":");
	  /* FIXME: handle multiple ID file names */
	}
    }
  if (arg == 0)
    arg = ID_FILE_NAME;

  /* if we got absolute name, just use it. */
  if (arg[0] == '/')
    return arg;
  /* if the name we were give exists, don't bother searching */
  if (stat (arg, &statb) == 0)
    return arg;
  /* search up the tree until we find a directory where this
	 * relative file name is visible.
	 * (or we run out of tree to search by hitting root).
	 */

  if (stat ("/", &rootb) != 0)
    return NULL;
  do
    {
      strcpy (buf, "../");
      buf += 3;
      strcpy (buf, arg);
      if (stat (file_name_buffer, &statb) == 0)
	return file_name_buffer;
      *buf = '\0';
      if (stat (file_name_buffer, &statb) != 0)
	return NULL;
    }
  while (!((statb.st_ino == rootb.st_ino) ||
	   (statb.st_dev == rootb.st_dev)));
  return NULL;
}

/* define special name components */

static char slash[] = "/";
static char dot[] = ".";
static char dotdot[] = "..";

/* nextc points to the next character to look at in the string or is
 * null if the end of string was reached.
 *
 * namep points to buffer that holds the components.
 */
static char const *nextc = NULL;
static char *namep;

/* lexname - Return next name component. Uses global variables initialized
 * by canonize_file_name to figure out what it is scanning.
 */
static char const *
lexname (void)
{
  char c;
  char const *d;

  if (nextc)
    {
      c = *nextc++;
      if (c == '\0')
	{
	  nextc = NULL;
	  return NULL;
	}
      if (c == '/')
	{
	  return &slash[0];
	}
      if (c == '.')
	{
	  if ((*nextc == '/') || (*nextc == '\0'))
	    return &dot[0];
	  if (*nextc == '.' && (*(nextc + 1) == '/' || *(nextc + 1) == '\0'))
	    {
	      ++nextc;
	      return &dotdot[0];
	    }
	}
      d = namep;
      *namep++ = c;
      while ((c = *nextc) != '/')
	{
	  *namep++ = c;
	  if (c == '\0')
	    {
	      nextc = NULL;
	      return d;
	    }
	  ++nextc;
	}
      *namep++ = '\0';
      return d;
    }
  else
    {
      return NULL;
    }
}

/* canonize_file_name - Put a file name in cannonical form. Looks for all the
 * whacky wonderful things a demented *ni* programmer might put
 * in a file name and reduces the name to cannonical form.
 */
void
canonize_file_name (char *n)
{
  char const *components[1024];
  char const **cap = &components[0];
  char const **cad;
  char const *cp;
  char namebuf[2048];
  char const *s;

  /* initialize scanner */
  nextc = n;
  namep = &namebuf[0];

  /* break the file name into individual components */
  while ((cp = lexname ()))
    {
      *cap++ = cp;
    }

  /* If name is empty, leave it that way */
  if (cap == &components[0])
    return;

  /* flag end of component list */
  *cap = NULL;

  /* remove all trailing slashes and dots */
  while ((--cap != &components[0]) &&
	 ((*cap == &slash[0]) || (*cap == &dot[0])))
    *cap = NULL;

  /* squeeze out all . / component sequences */
  cap = &components[0];
  cad = cap;
  while (*cap)
    {
      if ((*cap == &dot[0]) && (*(cap + 1) == &slash[0]))
	{
	  cap += 2;
	}
      else
	{
	  *cad++ = *cap++;
	}
    }
  *cad++ = NULL;

  /* find multiple // and use last slash as root, except on apollo which
    * apparently actually uses // in real file names (don't ask me why).
    */
#ifndef apollo
  s = NULL;
  cap = &components[0];
  cad = cap;
  while (*cap)
    {
      if ((s == &slash[0]) && (*cap == &slash[0]))
	{
	  cad = &components[0];
	}
      s = *cap++;
      *cad++ = s;
    }
  *cad = NULL;
#endif

  /* if this is absolute name get rid of any /.. at beginning */
  if ((components[0] == &slash[0]) && (components[1] == &dotdot[0]))
    {
      cap = &components[1];
      cad = cap;
      while (*cap == &dotdot[0])
	{
	  ++cap;
	  if (*cap == NULL)
	    break;
	  if (*cap == &slash[0])
	    ++cap;
	}
      while (*cap)
	*cad++ = *cap++;
      *cad = NULL;
    }

  /* squeeze out any name/.. sequences (but leave leading ../..) */
  cap = &components[0];
  cad = cap;
  while (*cap)
    {
      if ((*cap == &dotdot[0]) &&
	  ((cad - 2) >= &components[0]) &&
	  ((*(cad - 2)) != &dotdot[0]))
	{
	  cad -= 2;
	  ++cap;
	  if (*cap)
	    ++cap;
	}
      else
	{
	  *cad++ = *cap++;
	}
    }
  /* squeezing out a trailing /.. can leave unsightly trailing /s */
  if ((cad >= &components[2]) && ((*(cad - 1)) == &slash[0]))
    --cad;
  *cad = NULL;
  /* if it was just name/.. it now becomes . */
  if (components[0] == NULL)
    {
      components[0] = &dot[0];
      components[1] = NULL;
    }

  /* re-assemble components */
  cap = &components[0];
  while ((s = *cap++))
    {
      while (*s)
	*n++ = *s++;
    }
  *n++ = '\0';
}

FILE *
open_source_FILE (char *file_name)
{
  FILE *source_FILE;

  source_FILE = fopen (file_name, "r");
  if (source_FILE == NULL)
    error (0, errno, _("can't open `%s'"), file_name);
  return source_FILE;
}

void
close_source_FILE (FILE *fp)
{
  fclose (fp);
}
