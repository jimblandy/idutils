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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <config.h>
#include "strxtra.h"
#include "filenames.h"
#include "misc.h"

/* relative_file_name takes two arguments:
 * 1) an absolute path name for a directory.
 *    (This name MUST have a trailing /).
 * 2) an absolute path name for a file.
 *
 * It looks for common components at the front of the file and
 * directory names and generates a relative path name for the file
 * (relative to the specified directory).
 *
 * This may result in a huge number of ../s if the names
 * have no components in common.
 *
 * The output from this concatenated with the input directory name
 * and run through span_file_name should result in the original input
 * absolute path name of the file.
 *
 * Examples:
 *  dir      arg                  return value
 *  /x/y/z/  /x/y/q/file      ->  ../q/file
 *  /x/y/z/  /q/t/p/file      ->  ../../../q/t/p/file
 *  /x/y/z/  /x/y/z/file      ->  file
 */
char const *
relative_file_name (char const *dir, char const *arg)
{
  char const *a;
  char const *d;
  char const *lasta;
  char const *lastd;
  static char file_name_buffer[BUFSIZ];
  char *buf = file_name_buffer;

  lasta = a = arg;
  lastd = d = dir;
  while (*a == *d)
    {
      if (*a == '/')
	{
	  lasta = a;
	  lastd = d;
	}
      ++a;
      ++d;
    }
  /* lasta and lastd now point to the last / in each
	 * file name where the leading file components were
	 * identical.
	 */
  ++lasta;
  ++lastd;
  /* copy a ../ into the buffer for each component of
	 * the directory that remains.
	 */

  while (*lastd != '\0')
    {
      if (*lastd == '/')
	{
	  strcpy (buf, "../");
	  buf += 3;
	}
      ++lastd;
    }
  /* now tack on remainder of arg */
  strcpy (buf, lasta);
  return file_name_buffer;
}

/* span_file_name accepts a directory name and a file name and returns
   a cannonical form of the full file name within that directory.  It
   gets rid of ./ and things like that.  If the file is an absolute
   name then the directory is ignored.  */
char const *
span_file_name (char const *dir, char const *arg)
{
  char *argptr;
  static char file_name_buffer[BUFSIZ];

  /* reduce directory to cannonical form */
  strcpy (file_name_buffer, dir);
  cannoname (file_name_buffer);
  /* tack the obilgatory / on the end */
  strcat (file_name_buffer, "/");
  /* stick file name in buffer after directory */
  argptr = file_name_buffer + strlen (file_name_buffer);
  strcpy (argptr, arg);
  /* and reduce it to cannonical form also */
  cannoname (argptr);
  /* If it is an absolute name, just return it */
  if (*argptr == '/')
    return argptr;
  /* otherwise, combine the names to cannonical form */
  cannoname (file_name_buffer);
  return file_name_buffer;
}

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
  struct stat rootb;
  struct stat statb;

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
 * by cannoname to figure out what it is scanning.
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

/* cannoname - Put a file name in cannonical form. Looks for all the
 * whacky wonderful things a demented *ni* programmer might put
 * in a file name and reduces the name to cannonical form.
 */
void
cannoname (char *n)
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

/* kshgetwd is a routine that acts just like getwd, but is optimized
 * for ksh users, taking advantage of the fact that ksh maintains
 * an environment variable named PWD holding path name of the
 * current working directory.
 *
 * The primary motivation for this is not really that it is algorithmically
 * simpler, but that it is much less likely to bother NFS if we can just
 * guess the name of the current working directory using the hint that
 * ksh maintains. Anything that avoids NFS gettar failed messages is
 * worth doing.
 */
char const *
kshgetwd (char *pathname)
{
  struct stat kshstat, dotstat;
  char kshname[MAXPATHLEN];
  char const *kshp;

  kshp = getenv ("PWD");
  if (kshp)
    {
      /* OK, there was a PWD environment variable */
      strcpy (kshname, kshp);
      if (unsymlink (kshname)
	  /* And we could resolve the symbolic links through it */
	  && kshname[0] == '/'
	  /* And the name we have is an absolute path name */
	  && stat (kshname, &kshstat) == 0
	  /* And we can stat the name */
	  && stat (".", &dotstat) == 0
	  /* And we can stat "." */
	  && (kshstat.st_dev == dotstat.st_dev)
	  && (kshstat.st_ino == dotstat.st_ino))
	/* By golly, that name is the same file as "." ! */
	return strcpy (pathname, kshname);
    }

  /* Oh well, something did not work out right, do it the hard way */
#if HAVE_GETCWD
  return getcwd (pathname, BUFSIZ);
#else
#if HAVE_GETWD
  return getwd (pathname);
#endif
#endif
}

/* unsymlink is a routine that resolves all symbolic links in
 * a file name, transforming a name to the "actual" file name
 * instead of the name in terms of symbolic links.
 *
 * If it can resolve all links and discover an actual file
 * it returns a pointer to its argument string and transforms
 * the argument in place to the actual name.
 *
 * If no such actual file exists, or for some reason the links
 * cannot be resolved, it returns a NULL pointer and leaves the
 * name alone.
 */
char const *
unsymlink (char *n)
{
  char newname[MAXPATHLEN];
  char partname[MAXPATHLEN];
  char linkname[MAXPATHLEN];
  char const *s;
  char *d;
  char *lastcomp;
  int linksize;
  struct stat statb;

  /* Just stat the file to automagically do all the symbolic
    * link verification checks and make sure we have access to
    * directories, etc.
    */
  if (stat (n, &statb) != 0)
    return NULL;
  strcpy (newname, n);
  /* Now loop, lstating each component to see if it is a symbolic
    * link. For symbolic link components, use readlink() to get
    * the real name, put the read link name in place of the
    * last component, and start again.
    */
  cannoname (newname);
  s = &newname[0];
  d = &partname[0];
  if (*s == '/')
    *d++ = *s++;
  lastcomp = d;
  for (;;)
    {
      if ((*s == '/') || (*s == '\0'))
	{
	  /* we have a complete component name in partname, check it out */
	  *d = '\0';
	  if (lstat (partname, &statb) != 0)
	    return NULL;
	  if ((statb.st_mode & S_IFMT) == S_IFLNK)
	    {
	      /* This much of name is a symbolic link, do a readlink
             * and tack the bits and pieces together
             */
	      linksize = readlink (partname, linkname, MAXPATHLEN);
	      if (linksize < 0)
		return NULL;
	      linkname[linksize] = '\0';
	      strcpy (lastcomp, linkname);
	      lastcomp += linksize;
	      strcpy (lastcomp, s);
	      strcpy (newname, partname);
	      cannoname (newname);
	      s = &newname[0];
	      d = &partname[0];
	      if (*s == '/')
		{
		  *d++ = *s++;
		}
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
  strcpy (n, newname);
  return n;
}

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
