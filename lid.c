/* lid.c -- primary query interface for mkid database
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <limits.h>
#include <regex.h>
#include "alloc.h"
#include "idfile.h"
#include "idarg.h"
#include "token.h"
#include "bitops.h"
#include "strxtra.h"
#include "misc.h"
#include "filenames.h"

typedef void (*doit_t) (char const *name, char **argv);

unsigned char *tree8_to_bits (unsigned char *bits_vec, unsigned char const *hits_tree8);
void tree8_to_bits_1 (unsigned char **bits_vec, unsigned char const **hits_tree8, int level);
char **tree8_to_argv (unsigned char const *hits_tree8);
char **bits_to_argv (unsigned char const *bits_vec);

static void usage (void);
void look_id (char const *name, char **argv);
void grep_id (char const *name, char **argv);
void edit_id (char const *name, char **argv);
int skip_to_argv (char **argv);
int find_plain (char const *arg, doit_t doit);
int find_anchor (char const *arg, doit_t doit);
int find_regexp (char const *arg, doit_t doit);
int find_number (char const *arg, doit_t doit);
int find_non_unique (unsigned int, doit_t doit);
int find_apropos (char const *arg, doit_t doit);
void parse_frequency_arg (char const *arg);
int frequency_wanted (char const *tok);
char const *strcpos (char const *s1, char const *s2);
char const *file_regexp (char const *name0, char const *left_delimit, char const *right_delimit);
off_t find_token (char const *token);
int is_regexp (char *name);
char **vec_to_argv (int const *vec);
int file_name_wildcard (char const *re, char const *fn);
int match_file_names (char const *re, doit_t doit);
int word_match (char const *name0, char const *line);
int radix (char const *name);
int stoi (char const *name);
int otoi (char const *name);
int dtoi (char const *name);
int xtoi (char const *name);
void savetty (void);
void restoretty (void);
void linetty (void);
void chartty (void);

enum radix {
  RADIX_OCT = 1,
  RADIX_DEC = 2,
  RADIX_HEX = 4,
  RADIX_ALL = RADIX_DEC | RADIX_OCT | RADIX_HEX,
};

#define	TOLOWER(c)	(isupper (c) ? tolower (c) : (c))
#define IS_ALNUM(c)	(isalnum (c) || (c) == '_')

#ifndef CRUNCH_DEFAULT
#define CRUNCH_DEFAULT 1
#endif

/* Sorry about all the globals, but it's really cleaner this way. */
FILE *id_FILE;
int merging;
int radix_arg;
int echo_on = 1;
int crunch_on = CRUNCH_DEFAULT;
int file_name_regexp = 0;
int match_base = 0;
char id_dir[BUFSIZ];
int tree8_levels;
unsigned int bits_vec_size;
char PWD_name[BUFSIZ];
struct idhead idh;
struct idarg *id_args;
int (*find_func) (char const *, doit_t);
unsigned short frequency_low = 1;
unsigned short frequency_high = USHRT_MAX;
char *buf;
char *buf2;
unsigned char *bits_vec;

char const *program_name;

static void
usage (void)
{
  fprintf (stderr, "Usage: %s [-f<file>] [-u<n>] [-r<dir>] [-mewdoxasknc] patterns...\n", program_name);
  exit (1);
}

int
main (int argc, char **argv)
{
  char const *id_file_name = IDFILE;
  doit_t doit = look_id;
  int force_merge = 0;
  unsigned int unique_limit = 0;
  int use_id_file_name = 1;
  int use_pwd_file_name = 0;
  int use_relative_file_name = 0;
  char const *REL_file_name = NULL;
  int (*forced_find_func) (char const *, doit_t) = NULL;

  program_name = basename ((argc--, *argv++));

  while (argc)
    {
      char const *arg = (argc--, *argv++);
      int op = *arg++;
      switch (op)
	{
	case '-':
	case '+':
	  break;
	default:
	  (argc++, --argv);
	  goto argsdone;
	}
      while (*arg)
	switch (*arg++)
	  {
	  case 'f':
	    id_file_name = arg;
	    goto nextarg;
	  case 'u':
	    unique_limit = stoi (arg);
	    goto nextarg;
	  case 'm':
	    force_merge = 1;
	    break;
	  case 'e':
	    forced_find_func = find_regexp;
	    file_name_regexp = 1;
	    break;
	  case 'w':
	    forced_find_func = find_plain;
	    break;
	  case 'd':
	    radix_arg |= RADIX_DEC;
	    break;
	  case 'o':
	    radix_arg |= RADIX_OCT;
	    break;
	  case 'x':
	    radix_arg |= RADIX_HEX;
	    break;
	  case 'a':
	    radix_arg |= RADIX_ALL;
	    break;
	  case 'F':
	    parse_frequency_arg (arg);
	    goto nextarg;
	  case 'k':
	    crunch_on = 0;
	    break;
	  case 'g':
	    crunch_on = 1;
	    break;
	  case 'n':
	    echo_on = 0;
	    break;
	  case 'c':
	    use_id_file_name = 0;
	    use_pwd_file_name = 1;
	    break;
	  case 'b':
	    match_base = 1;
	    break;
	  case 'r':
	    use_id_file_name = 0;
	    use_relative_file_name = 1;
	    REL_file_name = arg;
	    goto nextarg;
	  default:
	    usage ();
	  }
    nextarg:;
    }
argsdone:

  if (use_pwd_file_name && use_relative_file_name)
    {
      fprintf (stderr, "%s: please use only one of -c or -r\n", program_name);
      usage ();
    }
  /* Look for the ID database up the tree */
  id_file_name = look_up (id_file_name);
  if (id_file_name == NULL)
    {
      filerr ("open", id_file_name);
      exit (1);
    }
  /* Find out current directory to relate names to */
  if (kshgetwd (PWD_name) == NULL)
    {
      fprintf (stderr, "%s: cannot determine current directory\n", program_name);
      exit (1);
    }
  strcat (PWD_name, "/");
  /* Determine absolute path name that database files are relative to */
  if (use_id_file_name)
    {
      strcpy (id_dir, span_file_name (PWD_name, id_file_name));
      *(strrchr (id_dir, '/') + 1) = '\0';
    }
  else if (use_pwd_file_name)
    {
      strcpy (id_dir, PWD_name);
    }
  else
    {
      strcpy (id_dir, span_file_name (PWD_name, REL_file_name));
      strcat (id_dir, "/");
    }
  id_FILE = init_idfile (id_file_name, &idh, &id_args);
  if (id_FILE == NULL)
    {
      filerr ("open", id_file_name);
      exit (1);
    }
  bits_vec_size = (idh.idh_paths + 7) >> 3;
  tree8_levels = tree8_count_levels (idh.idh_paths);

  switch (program_name[0])
    {
    case 'a':
      forced_find_func = find_apropos;
      /*FALLTHROUGH*/
    case 'l':
      doit = look_id;
      break;
    case 'g':
      doit = grep_id;
      break;
    case 'e':
      doit = edit_id;
      break;
    case 'p':
      forced_find_func = match_file_names;
      doit = look_id;
      break;
    default:
      program_name = "[algep]id";
      usage ();
    }

  if (argc == 0)
    {
      (argc++, --argv);
      *(char const **)argv = ".";
    }

  while (argc)
    {
      long val = -1;
      char *arg = (argc--, *argv++);

      if (forced_find_func)
	find_func = forced_find_func;
      else if (radix (arg) && (val = stoi (arg)) >= 0)
	find_func = find_number;
      else if (is_regexp (arg))
	find_func = find_regexp;
      else if (arg[0] == '^')
	find_func = find_anchor;
      else
	find_func = find_plain;

      if ((doit == look_id && !force_merge)
	  || (find_func == find_number
	      && val > 7
	      && radix_arg != RADIX_DEC
	      && radix_arg != RADIX_OCT
	      && radix_arg != RADIX_HEX))
	merging = 0;
      else
	merging = 1;

      buf = malloc (idh.idh_buf_size);
      buf2 = malloc (idh.idh_buf_size);
      bits_vec = MALLOC (unsigned char, bits_vec_size);

      if (unique_limit)
	{
	  if (!find_non_unique (unique_limit, doit))
	    fprintf (stderr, "All identifiers are unique within the first %d characters\n", unique_limit);
	  exit (0);
	}
      else if (!(*find_func) (arg, doit))
	{
	  fprintf (stderr, "%s: not found\n", arg);
	  continue;
	}
    }
  exit (0);
}

void
look_id (char const *name, char **argv)
{
  char const *arg;
  char const *dir;
  int crunching = 0;

  if (echo_on)
    printf ("%-14s ", name);
  while (*argv)
    {
      arg = *argv++;
      if (*argv && crunch_on && can_crunch (arg, *argv))
	{
	  if (crunching)
	    printf (",%s", root_name (arg));
	  else
	    {
	      dir = dirname (arg);
	      if (dir && !strequ (dir, "."))
		printf ("%s/", dir);
	      printf ("{%s", root_name (arg));
	    }
	  crunching = 1;
	}
      else
	{
	  if (crunching)
	    printf (",%s}%s", root_name (arg), suff_name (arg));
	  else
	    fputs (arg, stdout);
	  crunching = 0;
	  if (*argv)
	    putchar (' ');
	}
    }
  putchar ('\n');
}

void
grep_id (char const *name, char **argv)
{
  FILE *gid_FILE;
  char const *gid_name;
  char line[BUFSIZ];
  char const *re = NULL;
  int line_number;

  if (merging)
    {
      re = file_regexp (name, "[^a-zA-Z0-9_]_*", "[^a-zA-Z0-9_]");
      if (re)
	{
	  char const *regexp_error = re_comp (re);
	  if (regexp_error)
	    {
	      fprintf (stderr, "%s: Syntax Error: %s (%s)\n", program_name, re, regexp_error);
	      return;
	    }
	}
    }

  line[0] = ' ';		/* sentry */
  while (*argv)
    {
      gid_FILE = fopen (gid_name = *argv++, "r");
      if (gid_FILE == NULL)
	{
	  filerr ("open", gid_name);
	  continue;
	}
      line_number = 0;
      while (fgets (&line[1], sizeof (line), gid_FILE))
	{
	  line_number++;
	  if (re)
	    {
		if (!re_exec (line))
		  continue;
	    }
	  else if (!word_match (name, line))
	    continue;
	  printf ("%s:%d: %s", gid_name, line_number, &line[1]);
	}
      fclose (gid_FILE);
    }
}

void
edit_id (char const *name, char **argv)
{
  char re_buffer[BUFSIZ];
  char ed_arg_buffer[BUFSIZ];
  char const *re;
  int c;
  int skip;
  static char const *editor;
  static char const *eid_arg;
  static char const *eid_right_del;
  static char const *eid_left_del;

  if (editor == NULL)
    {
      editor = getenv ("EDITOR");
      if (editor == NULL)
	{
	  editor = "vi";
	  eid_arg = "+1;/%s/";
	  eid_left_del = "\\<";
	  eid_right_del = "\\>";
	}
    }
  if (eid_left_del == NULL)
    {
      eid_arg = getenv ("EIDARG");
      eid_left_del = getenv ("EIDLDEL");
      if (eid_left_del == NULL)
	eid_left_del = "";
      eid_right_del = getenv ("EIDRDEL");
      if (eid_right_del == NULL)
	eid_right_del = "";
    }

  look_id (name, argv);
  savetty ();
  for (;;)
    {
      printf ("Edit? [y1-9^S/nq] ");
      fflush (stdout);
      chartty ();
      c = (getchar () & 0177);
      restoretty ();
      switch (TOLOWER (c))
	{
	case '/':
	case ('s' & 037):
	  putchar ('/');
	  skip = skip_to_argv (argv);
	  if (skip < 0)
	    continue;
	  argv += skip;
	  goto editit;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  putchar (c);
	  skip = c - '0';
	  break;
	case 'y':
	  putchar (c);
	  /*FALLTHROUGH*/
	case '\n':
	case '\r':
	  skip = 0;
	  break;
	case 'q':
	  putchar (c);
	  putchar ('\n');
	  exit (0);
	case 'n':
	  putchar (c);
	  putchar ('\n');
	  return;
	default:
	  putchar (c);
	  putchar ('\n');
	  continue;
	}

      putchar ('\n');
      while (skip--)
	if (*++argv == NULL)
	  continue;
      break;
    }
editit:

  if (merging)
    re = file_regexp (name, eid_left_del, eid_right_del);
  else
    re = NULL;
  if (re == NULL)
    {
      re = re_buffer;
      sprintf (re_buffer, "%s%s%s", eid_left_del, name, eid_right_del);
    }

  switch (fork ())
    {
    case -1:
      fprintf (stderr, "%s: Cannot fork (%s)\n", program_name, strerror (errno));
      exit (1);
    case 0:
      argv--;
      if (eid_arg)
	{
	  argv--;
	  sprintf (ed_arg_buffer, eid_arg, re);
	  argv[1] = ed_arg_buffer;
	}
      *(char const **) argv = editor;
      execvp (editor, argv);
      filerr ("exec", editor);
    default:
      {
	void (*oldint) (int) = signal (SIGINT, SIG_IGN);
	void (*oldquit) (int) = signal (SIGQUIT, SIG_IGN);

	while (wait (0) == -1 && errno == EINTR)
	  ;

	signal (SIGINT, oldint);
	signal (SIGQUIT, oldquit);
      }
      break;
    }
}

int
skip_to_argv (char **argv)
{
  char pattern[BUFSIZ];
  unsigned int count;

  if (gets (pattern) == NULL)
    return -1;

  for (count = 0; *argv; count++, argv++)
    if (strcpos (*argv, pattern))
      return count;
  return -1;
}

int
find_plain (char const *arg, doit_t doit)
{
  if (find_token (arg) == 0)
    return 0;
  gets_past_00 (buf, id_FILE);
  assert (*buf);
  if (!frequency_wanted (buf))
    return 0;
  (*doit) (buf, tree8_to_argv (tok_hits_addr (buf)));
  return 1;
}

int
find_anchor (char const *arg, doit_t doit)
{
  int count;
  unsigned int length;

  if (find_token (++arg) == 0)
    return 0;

  length = strlen (arg);
  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (buf, id_FILE) > 0)
    {
      assert (*buf);
      if (!frequency_wanted (buf))
	continue;
      if (!strnequ (arg, buf, length))
	break;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (buf));
      else
	(*doit) (buf, tree8_to_argv (tok_hits_addr (buf)));
      count++;
    }
  if (merging && count)
    (*doit) (--arg, bits_to_argv (bits_vec));

  return count;
}

int
find_regexp (char const *re, doit_t doit)
{
  int count;
  char const *regexp_error;

  regexp_error = re_comp (re);
  if (regexp_error)
    {
      fprintf (stderr, "%s: Syntax Error: %s (%s)\n", program_name, re, regexp_error);
      return 0;
    }
  fseek (id_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (buf, id_FILE) > 0)
    {
      assert (*buf);
      if (!frequency_wanted (buf))
	continue;
      if (!re_exec (buf))
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (buf));
      else
	(*doit) (buf, tree8_to_argv (tok_hits_addr (buf)));
      count++;
    }
  if (merging && count)
    (*doit) (re, bits_to_argv (bits_vec));

  return count;
}

int
find_number (char const *arg, doit_t doit)
{
  int count;
  int rdx;
  int val;
  int hit_digits = 0;

  rdx = (val = stoi (arg)) ? RADIX_ALL : radix (arg);
  fseek (id_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (buf, id_FILE) > 0)
    {
      if (hit_digits)
	{
	  if (!isdigit (*buf))
	    break;
	}
      else
	{
	  if (isdigit (*buf))
	    hit_digits = 1;
	}

      if (!((radix_arg ? radix_arg : rdx) & radix (buf))
	  || stoi (buf) != val)
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (buf));
      else
	(*doit) (buf, tree8_to_argv (tok_hits_addr (buf)));
      count++;
    }
  if (merging && count)
    (*doit) (arg, bits_to_argv (bits_vec));

  return count;
}

/* Find identifiers that are non-unique within the first `count'
   characters.  */
int
find_non_unique (unsigned int limit, doit_t doit)
{
  char *old = buf;
  char *new = buf2;
  int consecutive = 0;
  int count = 0;
  char name[1024];

  if (limit <= 1)
    usage ();
  assert (limit < sizeof(name));

  name[0] = '^';
  *new = '\0';
  fseek (id_FILE, idh.idh_tokens_offset, SEEK_SET);
  while (gets_past_00 (old, id_FILE) > 0)
    {
      char *tmp;
      if (!(tok_flags (old) & TOK_NAME))
	continue;
      tmp = old;
      old = new;
      new = tmp;
      if (!strnequ (new, old, limit))
	{
	  if (consecutive && merging)
	    {
	      strncpy (&name[1], old, limit);
	      (*doit) (name, bits_to_argv (bits_vec));
	    }
	  consecutive = 0;
	  continue;
	}
      if (!consecutive++)
	{
	  if (merging)
	    tree8_to_bits (bits_vec, tok_hits_addr (old));
	  else
	    (*doit) (old, tree8_to_argv (tok_hits_addr (old)));
	  count++;
	}
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (new));
      else
	(*doit) (new, tree8_to_argv (tok_hits_addr (new)));
      count++;
    }
  if (consecutive && merging)
    {
      strncpy (&name[1], new, limit);
      (*doit) (name, bits_to_argv (bits_vec));
    }
  return count;
}

int
find_apropos (char const *arg, doit_t doit)
{
  int count;

  fseek (id_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (buf, id_FILE) > 0)
    {
      assert (*buf);
      if (!frequency_wanted (buf))
	continue;
      if (strcpos (buf, arg) == NULL)
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (buf));
      else
	(*doit) (buf, tree8_to_argv (tok_hits_addr (buf)));
      count++;
    }
  if (merging && count)
    (*doit) (arg, bits_to_argv (bits_vec));

  return count;
}

void
parse_frequency_arg (char const *arg)
{
  if (*arg == '-')
    frequency_low = 1;
  else
    {
      frequency_low = atoi (arg);
      while (isdigit (*arg))
	arg++;
      if (*arg == '-')
	arg++;
    }
  if (*arg)
    frequency_high = atoi (arg);
  else if (arg[-1] == '-')
    frequency_high = USHRT_MAX;
  else
    frequency_high = frequency_low;
  if (frequency_low > frequency_high)
    fprintf (stderr, "Bogus frequencies: %u > %u\n", frequency_low, frequency_high);
}

int
frequency_wanted (char const *tok)
{
  unsigned int count = tok_count (tok);
  return (frequency_low <= count && count <= frequency_high);
}

/* if string `s2' occurs in `s1', return a pointer to the first match.
   Ignore differences in alphabetic case.  */
char const *
strcpos (char const *s1, char const *s2)
{
  char const *s1p;
  char const *s2p;
  char const *s1last;

  for (s1last = &s1[strlen (s1) - strlen (s2)]; s1 <= s1last; s1++)
    for (s1p = s1, s2p = s2; TOLOWER (*s1p) == TOLOWER (*s2p); s1p++)
      if (*++s2p == '\0')
	return s1;
  return NULL;
}

/* Convert the regular expression that we used to locate identifiers
   in the id database into one suitable for locating the identifiers
   in files.  */
char const *
file_regexp (char const *name0, char const *left_delimit, char const *right_delimit)
{
  static char re_buffer[BUFSIZ];
  char *name = (char *) name0;

  if (find_func == find_number && merging)
    {
      sprintf (re_buffer, "%s0*[Xx]*0*%d[Ll]*%s", left_delimit, stoi (name), right_delimit);
      return re_buffer;
    }

  if (!is_regexp (name) && name[0] != '^')
    return NULL;

  if (name[0] == '^')
    name0++;
  else
    left_delimit = "";
  while (*++name)
    ;
  if (*--name == '$')
    *name = '\0';
  else
    right_delimit = "";

  sprintf (re_buffer, "%s%s%s", left_delimit, name0, right_delimit);
  return re_buffer;
}

off_t
find_token (char const *token_0)
{
  off_t offset = 0;
  off_t start = idh.idh_tokens_offset - 2;
  off_t end = idh.idh_end_offset;
  off_t anchor_offset = 0;
  int order = -1;

  while (start < end)
    {
      int c;
      int incr = 1;
      char const *token;

      offset = start + (end - start) / 2;
      fseek (id_FILE, offset, SEEK_SET);
      offset += skip_past_00 (id_FILE);
      if (offset >= end)
	{
	  offset = start + 2;
	  fseek (id_FILE, offset, SEEK_SET);
	}

      /* compare the token names */
      token = token_0;
      while (*token == (c = getc (id_FILE)) && *token && c)
	{
	  token++;
	  incr++;
	}
      if (c && !*token && find_func == find_anchor)
	anchor_offset = offset;
      order = *token - c;

      if (order < 0)
	end = offset - 2;
      else if (order > 0)
	start = offset + incr + skip_past_00 (id_FILE) - 2;
      else
	break;
    }

  if (order)
    {
      if (anchor_offset)
	offset = anchor_offset;
      else
	return 0;
    }
  fseek (id_FILE, offset, SEEK_SET);
  return offset;
}

/* Are there any regexp meta-characters in name?? */
int
is_regexp (char *name)
{
  int backslash = 0;

  if (*name == '^')
    name++;
  while (*name)
    {
      if (*name == '\\')
	{
	  if (strchr ("<>", name[1]))
	    return 1;
	  name++, backslash++;
	}
      else if (strchr ("[]{}().*+^$", *name))
	return 1;
      name++;
    }
  if (backslash)
    while (*name)
      {
	if (*name == '\\')
	  strcpy (name, name + 1);
	name++;
      }
  return 0;
}

/* file_name_wildcard implements a simple pattern matcher that
   emulates the shell wild card capability.

   * - any string of chars
   ? - any char
   [] - any char in set (if first char is !, any not in set)
   \ - literal match next char */
int
file_name_wildcard (char const *re, char const *fn)
{
  int c;
  int i;
  char set[256];
  int revset;

  while ((c = *re++) != '\0')
    {
      if (c == '*')
	{
	  if (*re == '\0')
	    return 1;		/* match anything at end */
	  while (*fn != '\0')
	    {
	      if (file_name_wildcard (re, fn))
		return 1;
	      ++fn;
	    }
	  return 0;
	}
      else if (c == '?')
	{
	  if (*fn++ == '\0')
	    return 0;
	}
      else if (c == '[')
	{
	  c = *re++;
	  memset (set, 0, 256);
	  if (c == '!')
	    {
	      revset = 1;
	      c = *re++;
	    }
	  else
	    revset = 0;
	  while (c != ']')
	    {
	      if (c == '\\')
		c = *re++;
	      set[c] = 1;
	      if ((*re == '-') && (*(re + 1) != ']'))
		{
		  re += 1;
		  while (++c <= *re)
		    set[c] = 1;
		  ++re;
		}
	      c = *re++;
	    }
	  if (revset)
	    for (i = 1; i < 256; ++i)
	      set[i] = !set[i];
	  if (!set[(int)*fn++])
	    return 0;
	}
      else
	{
	  if (c == '\\')
	    c = *re++;
	  if (c != *fn++)
	    return 0;
	}
    }
  return (*fn == '\0');
}

/* match_file_names implements the pid tool.  This matches the *names*
   of files in the database against the input pattern rather than the
   *contents* of the files.  */

int
match_file_names (char const *re, doit_t doit)
{
  char const *absname;
  struct idarg *ida = id_args;
  int i;
  int count = 0;
  int matched;

  if (file_name_regexp)
    {
      char const *regexp_error = re_comp (re);
      if (regexp_error)
	{
	  fprintf (stderr, "%s: Syntax Error: %s (%s)\n", program_name, re, regexp_error);
	  return 0;
	}
    }

  for (i = 0; i < idh.idh_paths; i++, ida++)
    {
      if (*ida->ida_arg == 0)
	continue;
      if (match_base)
	{
	  absname = strrchr (ida->ida_arg, '/');
	  if (absname == NULL)
	    absname = ida->ida_arg;
	}
      else
	absname = span_file_name (id_dir, ida->ida_arg);
      if (file_name_regexp)
	matched = re_exec (absname);
      else
	matched = file_name_wildcard (re, absname);
      if (matched)
	{
	  BITSET (bits_vec, i);
	  ++count;
	}
    }
  if (count)
    (*doit) (re, bits_to_argv (bits_vec));
  return count;
}

/* Does `name' occur in `line' delimited by non-alphanumerics?? */
int
word_match (char const *name0, char const *line)
{
  char const *name = name0;

  for (;;)
    {
      /* find an initial-character match */
      while (*line != *name)
	{
	  if (*line == '\n')
	    return 0;
	  line++;
	}
      /* do we have a word delimiter on the left ?? */
      if (isalnum (line[-1]))
	{
	  line++;
	  continue;
	}
      /* march down both strings as long as we match */
      while (*++name == *++line)
	;
      /* is this the end of `name', is there a word delimiter ?? */
      if (*name == '\0' && !IS_ALNUM (*line))
	return 1;
      name = name0;
    }
}

/* Use the C lexical rules to determine an ascii number's radix.  The
   radix is returned as a bit map, so that more than one radix may
   apply.  In particular, it is impossible to determine the radix of
   0, so return all possibilities.  */
int
radix (char const *name)
{
  if (!isdigit (*name))
    return 0;
  if (*name != '0')
    return RADIX_DEC;
  name++;
  if (*name == 'x' || *name == 'X')
    return RADIX_HEX;
  while (*name && *name == '0')
    name++;
  return (RADIX_OCT | ((*name) ? 0 : RADIX_DEC));
}

/* Convert an ascii string number to an integer.  Determine the radix
   before converting.  */
int
stoi (char const *name)
{
  switch (radix (name))
    {
    case RADIX_DEC:
      return (dtoi (name));
    case RADIX_OCT:
      return (otoi (&name[1]));
    case RADIX_HEX:
      return (xtoi (&name[2]));
    case RADIX_DEC | RADIX_OCT:
      return 0;
    default:
      return -1;
    }
}

/* Convert an ascii octal number to an integer. */
int
otoi (char const *name)
{
  int n = 0;

  while (*name >= '0' && *name <= '7')
    {
      n *= 010;
      n += *name++ - '0';
    }
  if (*name == 'l' || *name == 'L')
    name++;
  return (*name ? -1 : n);
}

/* Convert an ascii decimal number to an integer. */
int
dtoi (char const *name)
{
  int n = 0;

  while (isdigit (*name))
    {
      n *= 10;
      n += *name++ - '0';
    }
  if (*name == 'l' || *name == 'L')
    name++;
  return (*name ? -1 : n);
}

/* Convert an ascii hex number to an integer. */
int
xtoi (char const *name)
{
  int n = 0;

  while (isxdigit (*name))
    {
      n *= 0x10;
      if (isdigit (*name))
	n += *name++ - '0';
      else if (islower (*name))
	n += 0xa + *name++ - 'a';
      else
	n += 0xA + *name++ - 'A';
    }
  if (*name == 'l' || *name == 'L')
    name++;
  return (*name ? -1 : n);
}

unsigned char *
tree8_to_bits (unsigned char *bv_0, unsigned char const *hits_tree8)
{
  unsigned char* bv = bv_0;
  tree8_to_bits_1 (&bv, &hits_tree8, tree8_levels);
  return bv_0;
}

void
tree8_to_bits_1 (unsigned char **bv, unsigned char const **hits_tree8, int level)
{
  int hits = *(*hits_tree8)++;

  if (--level)
    {
      int incr = 1 << ((level - 1) * 3);
      int bit;
      for (bit = 1; bit & 0xff; bit <<= 1)
	{
	  if (bit & hits)
	    tree8_to_bits_1 (bv, hits_tree8, level);
	  else
	    *bv += incr;
	}
    }
  else
    *(*bv)++ |= hits;
}

char **
bits_to_argv (unsigned char const *bv)
{
  int const reserved_argv_slots = 3;
  static char **argv_0;
  char **argv;
  struct idarg *ida = id_args;
  struct idarg *end = &id_args[idh.idh_paths];

  if (argv_0 == NULL)
    argv_0 = MALLOC (char *, idh.idh_paths + reserved_argv_slots + 2);
  argv = &argv_0[reserved_argv_slots];

  for (;;)
    {
      int hits;
      int bit;

      while (*bv == 0)
	{
	  bv++;
	  ida += 8;
	  if (ida >= end)
	    goto out;
	}
      hits = *bv++;
      for (bit = 1; bit & 0xff; bit <<= 1)
	{
	  if (bit & hits)
	    {
	      if (!(ida->ida_flags & IDA_RELATIVE))
		{
		  char const *abs_name = span_file_name (id_dir, ida->ida_arg);
		  char const *rel_name = relative_file_name (PWD_name, abs_name);
		  char const *short_name = (strlen (rel_name) > strlen (abs_name)
					    ? abs_name : rel_name);
		  if (!strequ (short_name, ida->ida_arg))
		    ida->ida_arg = strdup (short_name);
		  ida->ida_flags |= IDA_RELATIVE;
		}
	      *argv++ = ida->ida_arg;
	    }
	  if (++ida >= end)
	    goto out;
	}
    }
out:
  *argv = NULL;
  return &argv_0[reserved_argv_slots];
}

char **
tree8_to_argv (unsigned char const *hits_tree8)
{
  memset (bits_vec, 0, bits_vec_size);
  return bits_to_argv (tree8_to_bits (bits_vec, hits_tree8));
}

#if HAVE_TERMIOS_H

#include <termios.h>
struct termios linemode;
struct termios charmode;
struct termios savemode;
#define GET_TTY_MODES(modes) tcgetattr (0, (modes))
#define SET_TTY_MODES(modes) tcsetattr(0, TCSANOW, (modes))

#else /* not HAVE_TERMIOS_H */

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_TERMIO_H
#include <termio.h>
struct termio linemode;
struct termio charmode;
struct termio savemode;
#define GET_TTY_MODES(modes) ioctl (0, TCGETA, (modes))
#define SET_TTY_MODES(modes) ioctl (0, TCSETA, (modes))

#else /* not HAVE_TERMIO_H */

#if HAVE_SGTTYB_H
#include <sgttyb.h>
struct sgttyb linemode;
struct sgttyb charmode;
struct sgttyb savemode;

#ifdef TIOCGETP
#define GET_TTY_MODES(modes) ioctl (0, TIOCGETP, (modes))
#define SET_TTY_MODES(modes) ioctl (0, TIOCSETP, (modes))
#else /* not TIOCGETP */
#define GET_TTY_MODES(modes) gtty (0, (modes))
#define SET_TTY_MODES(modes) stty (0, (modes))
#endif /* not TIOCGETP */

savetty()
{
#ifdef TIOCGETP
  ioctl(0, TIOCGETP, &savemode);
#else
  gtty(0, &savemode);
#endif
  charmode = linemode = savemode;

  charmode.sg_flags &= ~ECHO;
  charmode.sg_flags |= RAW;

  linemode.sg_flags |= ECHO;
  linemode.sg_flags &= ~RAW;
}

#endif /* HAVE_SGTTYB_H */
#endif /* not HAVE_TERMIO_H */
#endif /* not HAVE_TERMIOS_H */

#if HAVE_TERMIOS_H || HAVE_TERMIO_H

void
savetty (void)
{
  GET_TTY_MODES (&savemode);
  charmode = linemode = savemode;

  charmode.c_lflag &= ~(ECHO | ICANON | ISIG);
  charmode.c_cc[VMIN] = 1;
  charmode.c_cc[VTIME] = 0;

  linemode.c_lflag |= (ECHO | ICANON | ISIG);
  linemode.c_cc[VEOF] = 'd' & 037;
  linemode.c_cc[VEOL] = 0377;
}

#endif /* HAVE_TERMIOS_H || HAVE_TERMIO_H */

#if HAVE_TERMIOS_H || HAVE_TERMIO_H || HAVE_SGTTYB_H

void
restoretty (void)
{
  SET_TTY_MODES (&savemode);
}

void
linetty (void)
{
  SET_TTY_MODES (&linemode);
}

void
chartty (void)
{
  SET_TTY_MODES (&charmode);
}

#endif /* HAVE_TERMIOS_H || HAVE_TERMIO_H || HAVE_SGTTYB_H */
