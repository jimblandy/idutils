/* filenames.c -- file & directory name manipulations
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include "strxtra.h"
#include "filenames.h"
#include "misc.h"
#include "error.h"

#ifdef S_IFLNK
static char const *unsymlink __P((char *n));
#endif
static void canonical_name __P((char *n));
static char const *lex_name __P((void));
static int same_link __P((struct stat *x, struct stat *y));

FILE *popen ();

/* relative_file_name takes two arguments:
   1) an absolute path name for a directory. (*must* have a trailing "/").
   2) an absolute path name for a file.

   It looks for a common directory prefix and generates a name for the
   given file that is relative to the given directory.  The result
   might begin with a long sequence of "../"s, if the given names are
   long but have a short common prefix.
 
   (Note: If the the result of relative_file_name is appended to its
   directory argument and passed to span_file_name, span_file_name's
   result should match relative_file_name's file name argument.)
 
   Examples:
     dir      arg          return value
   /x/y/z/  /x/y/q/file  ../q/file
   /x/y/z/  /q/t/p/file  ../../../q/t/p/file
   /x/y/z/  /x/y/z/file  file */

char const *
relative_file_name (char const *dir_name, char const *file_name)
{
  static char file_name_buffer[MAXPATHLEN];
  char *bp = file_name_buffer;

  while (*file_name && *file_name++ == *dir_name++)
    ;
  while (*--dir_name != '/')
    ;
  dir_name++;
  while (*--file_name != '/')
    ;
  file_name++;
  /* file_name and dir_name now point past their common directory prefix */

  /* copy "../" into the buffer for each component of the directory
     that remains.  */

  while (*dir_name)
    {
      if (*dir_name++ == '/')
	{
	  strcpy (bp, "../");
	  bp += 3;
	}
    }

  strcpy (bp, file_name);
  return file_name_buffer;
}

/* span_file_name accepts a canonical directory name and a file name
   and returns a canonical path to the file name relative to the
   directory.  If the file name is absolute, then the directory is
   ignored.  */

char const *
span_file_name (char const *dir_name, char const *file_name)
{
  char *fnp;
  static char file_name_buffer[MAXPATHLEN];

  strcpy (file_name_buffer, dir_name);
  fnp = file_name_buffer + strlen (file_name_buffer);
  *fnp++ = '/';
  strcpy (fnp, file_name);
  canonical_name (fnp);
  /* If it is an absolute name, just return it */
  if (*fnp == '/')
    return fnp;
  /* otherwise, combine the names to canonical form */
  canonical_name (file_name_buffer);
  return file_name_buffer;
}

/* root_name strips off the directory prefix and one suffix.  If there
   is neither prefix nor suffix, (i.e., "/"), it returns the empty
   string.  */

char const *
root_name (char const *path)
{
  static char file_name_buffer[MAXPATHLEN];
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

/* suff_name returns the suffix (including the dot), or the
   empty-string if there is none.  */

char const *
suff_name (char const *path)
{
  char const *dot;

  dot = strrchr (path, '.');
  if (dot == NULL)
    return "";
  return dot;
}

/* Return non-zero if the two stat bufs refer to the same file or
   directory */

static int
same_link (struct stat *x, struct stat *y)
{
  return ((x->st_ino == y->st_ino) && (x->st_dev == y->st_dev));
}

/* find_id_file adds "../"s to the beginning of a file name until it
   finds the one that really exists.  If the file name starts with
   "/", just return it as is.  If we fail for any reason, report the
   error and exit.  */

char const *
find_id_file (char const *arg)
{
  static char file_name_buffer[MAXPATHLEN];
  char *name;
  char *dir_end;
  struct stat root_buf;
  struct stat stat_buf;

  if (arg[0] == '/')
    return arg;
  if (stat (arg, &stat_buf) == 0)
    return arg;

  name = &file_name_buffer[sizeof (file_name_buffer) - strlen (arg) - 1];
  strcpy (name, arg);
  dir_end = name - 1;

  if (stat ("/", &root_buf) < 0)
    {
      error (1, errno, "Can't stat `/'");
      return NULL;
    }
  do
    {
      *--name = '/';
      *--name = '.';
      *--name = '.';
      if (stat (name, &stat_buf) == 0)
	return name;
      *dir_end = '\0';
      if (stat (name, &stat_buf) < 0)
	return NULL;
      *dir_end = '/';
    }
  while (name >= &file_name_buffer[3] && !same_link(&stat_buf, &root_buf));
  error (1, errno, "Can't stat `%s' anywhere between here and `/'", arg);
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

/* lex_name - Return next name component. Uses global variables initialized
 * by canonical_name to figure out what it is scanning.
 */
static char const *
lex_name (void)
{
  char c;
  char const *d;

  if (nextc == NULL)
    return NULL;

  c = *nextc++;
  if (c == '\0')
    {
      nextc = NULL;
      return NULL;
    }
  if (c == '/')
    return slash;
  if (c == '.')
    {
      if ((*nextc == '/') || (*nextc == '\0'))
	return dot;
      if (*nextc == '.' && (*(nextc + 1) == '/' || *(nextc + 1) == '\0'))
	{
	  ++nextc;
	  return dotdot;
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

/* canonical_name puts a file name in canonical form.  It looks for all
   the whacky wonderful things a demented *ni* programmer might put in
   a file name and reduces the name to canonical form.  */

static void
canonical_name (char *file_name)
{
  char const *components[1024];
  char const **cap = components;
  char const **cad;
  char const *cp;
  char name_buf[2048];
  char const *s;

  /* initialize scanner */
  nextc = file_name;
  namep = name_buf;

  while ((cp = lex_name ()))
    *cap++ = cp;
  if (cap == components)
    return;
  *cap = NULL;

  /* remove all trailing slashes and dots */
  while ((--cap != components) &&
	 ((*cap == slash) || (*cap == dot)))
    *cap = NULL;

  /* squeeze out all "./" sequences */
  cad = cap = components;
  while (*cap)
    {
      if ((*cap == dot) && (*(cap + 1) == slash))
	cap += 2;
      else
	*cad++ = *cap++;
    }
  *cad++ = NULL;

  /* find multiple // and use last slash as root, except on apollo which
     apparently actually uses // in real file names (don't ask me why). */
#ifndef apollo
  s = NULL;
  cad = cap = components;
  while (*cap)
    {
      if ((s == slash) && (*cap == slash))
	cad = components;
      s = *cap++;
      *cad++ = s;
    }
  *cad = NULL;
#endif

  /* if this is absolute name get rid of any /.. at beginning */
  if ((components[0] == slash) && (components[1] == dotdot))
    {
      cad = cap = &components[1];
      while (*cap == dotdot)
	{
	  ++cap;
	  if (*cap == NULL)
	    break;
	  if (*cap == slash)
	    ++cap;
	}
      while (*cap)
	*cad++ = *cap++;
      *cad = NULL;
    }

  /* squeeze out any name/.. sequences (but leave leading ../..) */
  cap = components;
  cad = cap;
  while (*cap)
    {
      if ((*cap == dotdot) && ((cad - 2) >= components) && (*(cad - 2) != dotdot))
	{
	  cad -= 2;
	  ++cap;
	  if (*cap)
	    ++cap;
	}
      else
	*cad++ = *cap++;
    }
  /* squeezing out a trailing /.. can leave unsightly trailing /s */
  if ((cad >= &components[2]) && ((*(cad - 1)) == slash))
    --cad;
  *cad = NULL;
  /* if it was just name/.. it now becomes . */
  if (components[0] == NULL)
    {
      components[0] = dot;
      components[1] = NULL;
    }

  /* re-assemble components */
  cap = components;
  while ((s = *cap++))
    {
      while (*s)
	*file_name++ = *s++;
    }
  *file_name++ = '\0';
}

/* get_PWD is an optimized getwd(3) or getcwd(3) that takes advantage
   of the shell's $PWD environment-variable, if present.  This is
   particularly worth doing on NFS mounted filesystems.  */

char const *
get_PWD (char *pwd_buf)
{
  struct stat pwd_stat;
  struct stat dot_stat;
  char *pwd = getenv ("PWD");

  if (pwd)
    {
      pwd = strcpy (pwd_buf, pwd);
      if (pwd[0] != '/'
	  || stat (".", &dot_stat) < 0
	  || stat (pwd, &pwd_stat) < 0
	  || !same_link(&pwd_stat, &dot_stat)
#ifdef S_IFLNK
	  || !unsymlink (pwd)
	  || pwd[0] != '/'
	  || stat (pwd, &pwd_stat) < 0
	  || !same_link(&pwd_stat, &dot_stat)
#endif
	  )
	pwd = 0;
    }

  if (pwd == 0)
    {
      /* Oh well, something did not work out right, so do it the hard way... */
#if HAVE_GETCWD
      pwd = getcwd (pwd_buf, MAXPATHLEN);
#else
#if HAVE_GETWD
      pwd = getwd (pwd_buf);
#endif
#endif
    }
  if (pwd)
    strcat (pwd, "/");
  else
    error (1, errno, "Can't determine current working directory!");

  return pwd;
}

#ifdef S_IFLNK

/* unsymlink resolves all symbolic links in a file name into hard
   links.  If successful, it returns its argument and transforms
   the file name in situ.  If unsuccessful, it returns NULL, and leaves
   the argument untouched.  */

static char const *
unsymlink (char *file_name_buf)
{
  char new_buf[MAXPATHLEN];
  char part_buf[MAXPATHLEN];
  char link_buf[MAXPATHLEN];
  char const *s;
  char *d;
  char *lastcomp;
  struct stat stat_buf;

  strcpy (new_buf, file_name_buf);

  /* Now loop, lstating each component to see if it is a symbolic
     link.  For symbolic link components, use readlink() to get the
     real name, put the read link name in place of the last component,
     and start again.  */

  canonical_name (new_buf);
  s = new_buf;
  d = part_buf;
  if (*s == '/')
    *d++ = *s++;
  lastcomp = d;
  for (;;)
    {
      if ((*s == '/') || (*s == '\0'))
	{
	  /* we have a complete component name in partname, check it out */
	  *d = '\0';
	  if (lstat (part_buf, &stat_buf) < 0)
	    return NULL;
	  if ((stat_buf.st_mode & S_IFMT) == S_IFLNK)
	    {
	      /* This much of name is a symbolic link, do a readlink
		 and tack the bits and pieces together */
	      int link_size = readlink (part_buf, link_buf, MAXPATHLEN);
	      if (link_size < 0)
		return NULL;
	      link_buf[link_size] = '\0';
	      strcpy (lastcomp, link_buf);
	      lastcomp += link_size;
	      strcpy (lastcomp, s);
	      strcpy (new_buf, part_buf);
	      canonical_name (new_buf);
	      s = new_buf;
	      d = part_buf;
	      if (*s == '/')
		*d++ = *s++;
	      lastcomp = d;
	    }
	  else
	    {
	      /* Not a symlink, just keep scanning to next component */
	      if (*s == '\0')
		break;
	      *d++ = *s++;
	      lastcomp = d;
	    }
	}
      else
	{
	  *d++ = *s++;
	}
    }
  strcpy (file_name_buf, new_buf);
  return file_name_buf;
}

#endif

FILE *
open_source_FILE (char *file_name, char const *filter)
{
  FILE *source_FILE;

  if (filter)
    {
      char command[1024];
      sprintf (command, filter, file_name);
      source_FILE = popen (command, "r");
    }
  else
    source_FILE = fopen (file_name, "r");
  if (source_FILE == NULL)
    filerr ("open", file_name);
  return source_FILE;
}

void
close_source_FILE (FILE *fp, char const *filter)
{
  if (filter)
    pclose (fp);
  else
    fclose (fp);
}
