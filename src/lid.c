/* lid.c -- primary query interface for mkid database
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <limits.h>
#if WITH_REGEX
# include <regex.h>
#else
# include <rx.h>
#endif

#include <config.h>
#include <getopt.h>
#include "system.h"
#include "alloc.h"
#include "idfile.h"
#include "token.h"
#include "bitops.h"
#include "strxtra.h"
#include "misc.h"
#include "filenames.h"
#include "error.h"
#include "pathmax.h"

typedef void (*report_func_t) __P((char const *name, struct file_link **flinkv));
typedef int (*query_func_t) __P((char const *arg, report_func_t));

unsigned char *tree8_to_bits __P((unsigned char *bits_vec, unsigned char const *hits_tree8));
void tree8_to_bits_1 __P((unsigned char **bits_vec, unsigned char const **hits_tree8, int level));
struct file_link **tree8_to_flinkv __P((unsigned char const *hits_tree8));
struct file_link **bits_to_flinkv __P((unsigned char const *bits_vec));

void usage __P((void));
static void help_me __P((void));
int common_prefix_suffix __P((struct file_link const *flink_1, struct file_link const *flink_2));
int member_file_index_qsort_compare __P((void const *x, void const *y));
void look_id __P((char const *name, struct file_link **flinkv));
void grep_id __P((char const *name, struct file_link **flinkv));
void edit_id __P((char const *name, struct file_link **flinkv));
int vector_cardinality __P((void *vector));
int skip_to_argv __P((struct file_link **flinkv));
int query_plain __P((char const *arg, report_func_t report_function));
int query_anchor __P((char const *arg, report_func_t report_function));
int query_regexp __P((char const *arg, report_func_t report_function));
int query_number __P((char const *arg, report_func_t report_function));
int query_non_unique __P((unsigned int, report_func_t report_function));
int query_apropos __P((char const *arg, report_func_t report_function));
void parse_frequency_arg __P((char const *arg));
int frequency_wanted __P((char const *tok));
char const *strcpos __P((char const *s1, char const *s2));
char const *file_regexp __P((char const *name0, char const *left_delimit, char const *right_delimit));
off_t query_token __P((char const *token));
int is_regexp __P((char *name));
int file_name_wildcard __P((char const *re, char const *fn));
int word_match __P((char const *name0, char const *line));
int get_radix __P((char const *name));
int stoi __P((char const *name));
int otoi __P((char const *name));
int dtoi __P((char const *name));
int xtoi __P((char const *name));
void savetty __P((void));
void restoretty __P((void));
void linetty __P((void));
void chartty __P((void));

enum radix {
  radix_oct = 1,
  radix_dec = 2,
  radix_hex = 4,
  radix_all = radix_dec | radix_oct | radix_hex
};

#define	TOLOWER(c)	(isupper (c) ? tolower (c) : (c))
#define IS_ALNUM(c)	(isalnum (c) || (c) == '_')

#ifndef BRACE_NOTATION_DEFAULT
#define BRACE_NOTATION_DEFAULT 1
#endif

/* Sorry about all the globals, but it's really cleaner this way. */

int merging;
int file_name_regexp = 0;
char anchor_dir[BUFSIZ];
int tree8_levels;
unsigned int bits_vec_size;
struct idhead idh;
char *hits_buf_1;
char *hits_buf_2;
unsigned char *bits_vec;

/* The name this program was run with. */

char const *program_name;

/* If nonzero, display usage information and exit.  */

static int show_help;

/* If nonzero, print the version on standard output then exit.  */

static int show_version;

/* Which radixes do we want? */

int radix_flag = radix_all;

/* If nonzero, don't print the name of the matched identifier.  */

int no_id_flag = 0;

/* If nonzero, merge multiple look_id regexp output lines into a
   single line.  */

int merge_flag = 0;

/* If nonzero, ignore differences in alphabetic case while matching.  */

int ignore_case_flag = 0;

/* If nonzero, print file names in abbreviated fashion using the
   shell's brace notation.  */

int brace_notation_flag = BRACE_NOTATION_DEFAULT;

/* If non-zero, list identifiers that are are non-unique within this
   number of leading characters.  */

unsigned int ambiguous_prefix_length = 0;

/* The file name of the ID database.  */

char const *id_file_name;

/* The style of report.  */

report_func_t report_function = look_id;

/* The style of query.  */

query_func_t query_func;

/* The style of query explicitly set by user from the command-line.  */

query_func_t forced_query_func;

/* Lower and upper bounds on occurrence frequency.  */

unsigned int frequency_low = 1;
unsigned int frequency_high = USHRT_MAX;

struct file_link *cw_dlink;
struct file_link **members_0;

static struct option const long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "frequency", required_argument, 0, 'F' },
  { "ambiguous", required_argument, 0, 'a' },
  { "grep", no_argument, 0, 'G' },
  { "apropos", no_argument, 0, 'A' },
  { "edit", no_argument, 0, 'E' },
  { "regexp", no_argument, 0, 'e' },
  { "braces", no_argument, 0, 'b' },
  { "merge", no_argument, 0, 'm' },
  { "ignore-case", no_argument, 0, 'i' },
  { "word", no_argument, 0, 'w' },
  { "hex", no_argument, 0, 'x' },
  { "decimal", no_argument, 0, 'd' },
  { "octal", no_argument, 0, 'o' },
  { "no-id", no_argument, 0, 'n' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { 0 }
};

void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (1);
}

static void
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... PATTERN...\n"),
	  program_name);
  printf (_("\
Query ID database and report results.\n\
By default, output consists of multiple lines, each line containing the\n\
matched identifier followed by the list of file names in which it occurs.\n\
\n\
  -f, --file=FILE      file name of ID database\n\
  -G, --grep           show every line where the matched identifier occurs\n\
  -E, --edit           edit every file where the matched identifier occurs\n\
  -m, --merge          output a multi-line regexp match as a single line\n\
  -n, --no-id          print file names only - omit the identifier\n\
  -b, --braces         toggle shell brace-notation for output file names\n\
\n\
If PATTERN contains regular expression metacharacters, it is interpreted\n\
as a regular expression.  Otherwise, PATTERN is interpreted as a literal\n\
word.\n\
\n\
  -e, --regexp         match PATTERN as a regular expression substring\n\
  -w, --word           match PATTERN as a word\n\
  -i, --ignore-case    match PATTERN case insinsitively\n\
  -A, --apropos        match PATTERN as a case-insensitive substring\n\
\n\
  -F, --frequency=FREQ find identifiers that occur FREQ times, where FREQ\n\
                       is a range expressed as `N..M'.  N omitted defaults\n\
                       to 1, M omitted defaults to MAX_USHRT.\n\
  -a, --ambiguous=LEN  find identifiers whose names are ambiguous for LEN chars\n\
\n\
  -x, --hex            only find numbers expressed as hexadecimal\n\
  -d, --decimal        only find numbers expressed as decimal\n\
  -o, --octal          only find numbers expressed as octal\n\
\n\
      --help           display this help and exit\n\
      --version        output version information and exit\n\
"));
  exit (0);
}

int
main (int argc, char **argv)
{
  program_name = argv[0];
  for (;;)
    {
      int optc = getopt_long (argc, argv, "f:F:a:GAEebmiwxdon",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'f':
	  id_file_name = optarg;
	  break;

	case 'F':
	  parse_frequency_arg (optarg);
	  break;

	case 'a':
	  ambiguous_prefix_length = stoi (optarg);
	  break;

	case 'G':
	  report_function = grep_id;
	  break;

	case 'A':
	  forced_query_func = query_apropos;
	  report_function = look_id;
	  break;
	    
	case 'E':
	  report_function = edit_id;
	  break;

	case 'e':
	  forced_query_func = query_regexp;
	  file_name_regexp = 1;
	  break;

	case 'b':
	  brace_notation_flag = !brace_notation_flag;
	  break;

	case 'm':
	  merge_flag = 1;
	  break;

	case 'i':
	  ignore_case_flag = REG_ICASE;
	  break;

	case 'w':
	  forced_query_func = query_plain;
	  break;

	case 'x':
	  radix_flag |= radix_hex;
	  break;

	case 'd':
	  radix_flag |= radix_dec;
	  break;

	case 'o':
	  radix_flag |= radix_oct;
	  break;

	case 'n':
	  no_id_flag = 1;
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

  /* Look for the ID database up the tree */
  id_file_name = look_up (id_file_name);
  if (id_file_name == 0)
    error (1, errno, _("can't locate `ID'"));
  
  init_idh_obstacks (&idh);
  init_idh_tables (&idh);

  cw_dlink = get_current_dir_link ();

  /* Determine absolute name of the directory name to which database
     constituent files are relative. */
  members_0 = read_id_file (id_file_name, &idh);
  bits_vec_size = (idh.idh_files + 7) / 4; /* more than enough */
  tree8_levels = tree8_count_levels (idh.idh_files);

  argc -= optind;
  argv += optind;
  if (argc == 0)
    {
      argc++;
      *(char const **)--argv = ".*";
    }

  while (argc)
    {
      long val = -1;
      char *arg = (argc--, *argv++);

      if (forced_query_func)
	query_func = forced_query_func;
      else if (get_radix (arg) && (val = stoi (arg)) >= 0)
	query_func = query_number;
      else if (is_regexp (arg))
	query_func = query_regexp;
      else if (arg[0] == '^')
	query_func = query_anchor;
      else
	query_func = query_plain;

      if ((report_function == look_id && !merge_flag)
	  || (query_func == query_number
	      && val > 7
	      && radix_flag != radix_dec
	      && radix_flag != radix_oct
	      && radix_flag != radix_hex))
	merging = 0;
      else
	merging = 1;

      hits_buf_1 = xmalloc (idh.idh_buf_size);
      hits_buf_2 = xmalloc (idh.idh_buf_size);
      bits_vec = MALLOC (unsigned char, bits_vec_size);

      if (ambiguous_prefix_length)
	{
	  if (!query_non_unique (ambiguous_prefix_length, report_function))
	    fprintf (stderr, _("All identifiers are non-ambiguous within the first %d characters\n"),
		     ambiguous_prefix_length);
	  exit (0);
	}
      else if (!(*query_func) (arg, report_function))
	{
	  fprintf (stderr, _("%s: not found\n"), arg);
	  continue;
	}
    }
  fclose (idh.idh_FILE);
  exit (0);
}

/* common_prefix_suffix returns non-zero if two file names have a
   fully common directory prefix and a common suffix (i.e., they're
   eligible for coalescing with brace notation.  */

int
common_prefix_suffix (struct file_link const *flink_1, struct file_link const *flink_2)
{
  return (flink_1->fl_parent == flink_2->fl_parent
	  && strequ (suff_name (flink_1->fl_name), suff_name (flink_2->fl_name)));
}

void
look_id (char const *name, struct file_link **flinkv)
{
  struct file_link const *arg;
  struct file_link const *dlink;
  int brace_is_open = 0;

  if (!no_id_flag)
    printf ("%-14s ", name);
  while (*flinkv)
    {
      arg = *flinkv++;
      if (*flinkv && brace_notation_flag
	  && common_prefix_suffix (arg, *flinkv))
	{
	  if (brace_is_open)
	    printf (",%s", root_name (arg->fl_name));
	  else
	    {
	      dlink = arg->fl_parent;
	      if (dlink && dlink != cw_dlink)
		{
		  char buf[PATH_MAX];
		  maybe_relative_path (buf, dlink, cw_dlink);
		  fputs (buf, stdout);
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
	      char buf[PATH_MAX];
	      maybe_relative_path (buf, arg, cw_dlink);
	      fputs (buf, stdout);
	    }
	  brace_is_open = 0;
	  if (*flinkv)
	    putchar (' ');
	}
    }
  putchar ('\n');
}

/* FIXME: use regcomp regexec */

void
grep_id (char const *name, struct file_link **flinkv)
{
  char line[BUFSIZ];
  char const *pattern = 0;
  regex_t compiled;
  int line_number;

  if (merging)
    {
      pattern = file_regexp (name, "[^a-zA-Z0-9_À-ÿ]_*", "[^a-zA-Z0-9_À-ÿ]");
      if (pattern)
	{
	  int regcomp_errno = regcomp (&compiled, pattern,
				       ignore_case_flag | REG_EXTENDED);
	  if (regcomp_errno)
	    {
	      char buf[BUFSIZ];
	      regerror (regcomp_errno, &compiled, buf, sizeof (buf));
	      error (1, 0, "%s", buf);
	    }
	}
    }

  line[0] = ' ';		/* sentry */
  while (*flinkv)
    {
      FILE *gid_FILE;
      char file_name[PATH_MAX];

      maybe_relative_path (file_name, *flinkv++, cw_dlink);
      gid_FILE = fopen (file_name, "r");
      if (gid_FILE == 0)
	error (0, errno, "can't open `%s'", file_name);

      line_number = 0;
      while (fgets (&line[1], sizeof (line), gid_FILE))
	{
	  line_number++;
	  if (pattern)
	    {
	      int regexec_errno = regexec (&compiled, line, 0, 0, 0);
	      if (regexec_errno == REG_ESPACE)
		error (0, 0, "can't match regular-expression: memory exhausted");
	      else if (regexec_errno)
		continue;
	    }
	  else if (!word_match (name, line))
	    continue;
	  printf ("%s:%d: %s", file_name, line_number, &line[1]);
	}
      fclose (gid_FILE);
    }
}

void
edit_id (char const *name, struct file_link **flinkv)
{
  static char const *editor;
  static char const *eid_arg;
  static char const *eid_right_del;
  static char const *eid_left_del;
  char re_buffer[BUFSIZ];
  char ed_arg_buffer[BUFSIZ];
  char const *pattern;
  int c;
  int skip;

  if (editor == 0)
    {
      editor = getenv ("EDITOR");
      if (editor == 0)
	editor = "vi";
    }

  if (eid_arg == 0)
    {
      int using_vi = strequ ("vi", basename (editor));

      eid_arg = getenv ("EIDARG");
      if (eid_arg == 0)
	eid_arg = (using_vi ? "+1;/%s/" : "");

      eid_left_del = getenv ("EIDLDEL");
      if (eid_left_del == 0)
	eid_left_del = (using_vi ? "\\<" : "");

      eid_right_del = getenv ("EIDRDEL");
      if (eid_right_del == 0)
	eid_right_del = (using_vi ? "\\>" : "");
    }

  look_id (name, flinkv);
  savetty ();
  for (;;)
    {
      /* FIXME: i18n */
      printf (_("Edit? [y1-9^S/nq] "));
      fflush (stdout);
      chartty ();
      c = (getchar () & 0177);
      restoretty ();
      switch (TOLOWER (c))
	{
	case '/':
	case ('s' & 037):
	  putchar ('/');
	  skip = skip_to_flinkv (flinkv);
	  if (skip < 0)
	    continue;
	  flinkv += skip;
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
	  skip = 0;
	  break;
	case '\n':
	case '\r':
	  putchar ('y');
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
	if (*++flinkv == 0)
	  continue;
      break;
    }
editit:

  if (merging)
    pattern = file_regexp (name, eid_left_del, eid_right_del);
  else
    pattern = 0;
  if (pattern == 0)
    {
      pattern = re_buffer;
      sprintf (re_buffer, "%s%s%s", eid_left_del, name, eid_right_del);
    }

  switch (fork ())
    {
    case -1:
      error (1, errno, _("can't fork"));
      break;

    case 0:
      {
	char **argv_0 = MALLOC (char *, 3 + vector_cardinality (flinkv));
	char **argv = argv_0 + 2;
	while (*flinkv)
	  {
	    char buf[PATH_MAX];
	    maybe_relative_path (buf, *flinkv++, cw_dlink);
	    *argv++ = strdup (buf);
	  }
	*argv = 0;
	argv = argv_0 + 1;
	if (eid_arg)
	  {
	    sprintf (ed_arg_buffer, eid_arg, pattern);
	    *--argv = ed_arg_buffer;
	  }
	*(char const **)argv = editor;
	execvp (editor, argv);
	error (0, errno, _("can't exec `%s'"), editor);
      }

    default:
      {
	void (*oldint) __P((int)) = signal (SIGINT, SIG_IGN);
	void (*oldquit) __P((int)) = signal (SIGQUIT, SIG_IGN);

	while (wait (0) == -1 && errno == EINTR)
	  ;

	signal (SIGINT, oldint);
	signal (SIGQUIT, oldquit);
      }
      break;
    }
}

int
vector_cardinality (void *vector)
{
  void **v = (void **)vector;
  int count = 0;

  while (*v++)
    count++;
  return count;
}

int
skip_to_flinkv (struct file_link **flinkv)
{
  char pattern[BUFSIZ];
  unsigned int count;

  if (gets (pattern) == 0)
    return -1;

  for (count = 0; *flinkv; count++, flinkv++)
    {
      char buf[PATH_MAX];
      maybe_relative_path (buf, *flinkv, cw_dlink);
      if (strcpos (buf, pattern))
	return count;
    }
  return -1;
}

int
query_plain (char const *arg, report_func_t report_function)
{
  if (query_token (arg) == 0)
    return 0;
  gets_past_00 (hits_buf_1, idh.idh_FILE);
  assert (*hits_buf_1);
  if (!frequency_wanted (hits_buf_1))
    return 0;
  (*report_function) (hits_buf_1, tree8_to_flinkv (tok_hits_addr (hits_buf_1)));
  return 1;
}

int
query_anchor (char const *arg, report_func_t report_function)
{
  int count;
  unsigned int length;

  if (query_token (++arg) == 0)
    return 0;

  length = strlen (arg);
  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (hits_buf_1, idh.idh_FILE) > 0)
    {
      assert (*hits_buf_1);
      if (!frequency_wanted (hits_buf_1))
	continue;
      if (!strnequ (arg, hits_buf_1, length))
	break;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (hits_buf_1));
      else
	(*report_function) (hits_buf_1, tree8_to_flinkv (tok_hits_addr (hits_buf_1)));
      count++;
    }
  if (merging && count)
    (*report_function) (--arg, bits_to_flinkv (bits_vec));

  return count;
}

int
query_regexp (char const *pattern, report_func_t report_function)
{
  int count;
  regex_t compiled;
  int regcomp_errno;

  regcomp_errno = regcomp (&compiled, pattern,
			   ignore_case_flag | REG_EXTENDED);
  if (regcomp_errno)
    {
      char buf[BUFSIZ];
      regerror (regcomp_errno, &compiled, buf, sizeof (buf));
      error (1, 0, "%s", buf);
    }
  fseek (idh.idh_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (hits_buf_1, idh.idh_FILE) > 0)
    {
      int regexec_errno;
      assert (*hits_buf_1);
      if (!frequency_wanted (hits_buf_1))
	continue;
      regexec_errno = regexec (&compiled, hits_buf_1, 0, 0, 0);
      if (regexec_errno == REG_ESPACE)
	error (0, 0, _("can't match regular-expression: memory exhausted"));
      else if (regexec_errno)
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (hits_buf_1));
      else
	(*report_function) (hits_buf_1, tree8_to_flinkv (tok_hits_addr (hits_buf_1)));
      count++;
    }
  if (merging && count)
    (*report_function) (pattern, bits_to_flinkv (bits_vec));

  return count;
}

int
query_number (char const *arg, report_func_t report_function)
{
  int count;
  int radix;
  int val;
  int hit_digits = 0;

  radix = (val = stoi (arg)) ? radix_all : get_radix (arg);
  fseek (idh.idh_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (hits_buf_1, idh.idh_FILE) > 0)
    {
      if (hit_digits)
	{
	  if (!isdigit (*hits_buf_1))
	    break;
	}
      else
	{
	  if (isdigit (*hits_buf_1))
	    hit_digits = 1;
	}

      if (!((radix_flag ? radix_flag : radix) & get_radix (hits_buf_1))
	  || stoi (hits_buf_1) != val)
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (hits_buf_1));
      else
	(*report_function) (hits_buf_1, tree8_to_flinkv (tok_hits_addr (hits_buf_1)));
      count++;
    }
  if (merging && count)
    (*report_function) (arg, bits_to_flinkv (bits_vec));

  return count;
}

/* Find identifiers that are non-unique within the first `count'
   characters.  */

int
query_non_unique (unsigned int limit, report_func_t report_function)
{
  char *old = hits_buf_1;
  char *new = hits_buf_2;
  int consecutive = 0;
  int count = 0;
  char name[1024];

  if (limit <= 1)
    usage ();
  assert (limit < sizeof(name));

  name[0] = '^';
  *new = '\0';
  fseek (idh.idh_FILE, idh.idh_tokens_offset, SEEK_SET);
  while (gets_past_00 (old, idh.idh_FILE) > 0)
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
	      (*report_function) (name, bits_to_flinkv (bits_vec));
	    }
	  consecutive = 0;
	  continue;
	}
      if (!consecutive++)
	{
	  if (merging)
	    tree8_to_bits (bits_vec, tok_hits_addr (old));
	  else
	    (*report_function) (old, tree8_to_flinkv (tok_hits_addr (old)));
	  count++;
	}
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (new));
      else
	(*report_function) (new, tree8_to_flinkv (tok_hits_addr (new)));
      count++;
    }
  if (consecutive && merging)
    {
      strncpy (&name[1], new, limit);
      (*report_function) (name, bits_to_flinkv (bits_vec));
    }
  return count;
}

int
query_apropos (char const *arg, report_func_t report_function)
{
  int count;

  fseek (idh.idh_FILE, idh.idh_tokens_offset, SEEK_SET);

  count = 0;
  if (merging)
    memset (bits_vec, 0, bits_vec_size);
  while (gets_past_00 (hits_buf_1, idh.idh_FILE) > 0)
    {
      assert (*hits_buf_1);
      if (!frequency_wanted (hits_buf_1))
	continue;
      if (strcpos (hits_buf_1, arg) == 0)
	continue;
      if (merging)
	tree8_to_bits (bits_vec, tok_hits_addr (hits_buf_1));
      else
	(*report_function) (hits_buf_1, tree8_to_flinkv (tok_hits_addr (hits_buf_1)));
      count++;
    }
  if (merging && count)
    (*report_function) (arg, bits_to_flinkv (bits_vec));

  return count;
}

void
parse_frequency_arg (char const *arg)
{
  if (strnequ (arg, "..", 2))
    frequency_low = 1;
  else
    {
      frequency_low = atoi (arg);
      while (isdigit (*arg))
	arg++;
      if (strnequ (arg, "..", 2))
	arg += 2;
    }
  if (*arg)
    frequency_high = atoi (arg);
  else if (strnequ (&arg[-1], "..", 2))
    frequency_high = USHRT_MAX;
  else
    frequency_high = frequency_low;
  if (frequency_low > frequency_high)
    {
      unsigned int tmp = frequency_low;
      frequency_low = frequency_high;
      frequency_high = tmp;
    }
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
  return 0;
}

/* Convert the regular expression that we used to locate identifiers
   in the id database into one suitable for locating the identifiers
   in files.  */

char const *
file_regexp (char const *name0, char const *left_delimit, char const *right_delimit)
{
  static char pat_buf[BUFSIZ];
  char *name = (char *) name0;

  if (query_func == query_number && merging)
    {
      sprintf (pat_buf, "%s0*[Xx]*0*%d[Ll]*%s", left_delimit, stoi (name), right_delimit);
      return pat_buf;
    }

  if (!is_regexp (name) && name[0] != '^')
    return 0;

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

  sprintf (pat_buf, "%s%s%s", left_delimit, name0, right_delimit);
  return pat_buf;
}

off_t
query_token (char const *token_0)
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
      fseek (idh.idh_FILE, offset, SEEK_SET);
      offset += skip_past_00 (idh.idh_FILE);
      if (offset >= end)
	{
	  offset = start + 2;
	  fseek (idh.idh_FILE, offset, SEEK_SET);
	}

      /* compare the token names */
      token = token_0;
      while (*token == (c = getc (idh.idh_FILE)) && *token && c)
	{
	  token++;
	  incr++;
	}
      if (c && !*token && query_func == query_anchor)
	anchor_offset = offset;
      order = *token - c;

      if (order < 0)
	end = offset - 2;
      else if (order > 0)
	start = offset + incr + skip_past_00 (idh.idh_FILE) - 2;
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
  fseek (idh.idh_FILE, offset, SEEK_SET);
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
file_name_wildcard (char const *pattern, char const *fn)
{
  int c;
  int i;
  char set[256];
  int revset;

  while ((c = *pattern++) != '\0')
    {
      if (c == '*')
	{
	  if (*pattern == '\0')
	    return 1;		/* match anything at end */
	  while (*fn != '\0')
	    {
	      if (file_name_wildcard (pattern, fn))
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
	  c = *pattern++;
	  memset (set, 0, 256);
	  if (c == '!')
	    {
	      revset = 1;
	      c = *pattern++;
	    }
	  else
	    revset = 0;
	  while (c != ']')
	    {
	      if (c == '\\')
		c = *pattern++;
	      set[c] = 1;
	      if ((*pattern == '-') && (*(pattern + 1) != ']'))
		{
		  pattern += 1;
		  while (++c <= *pattern)
		    set[c] = 1;
		  ++pattern;
		}
	      c = *pattern++;
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
	    c = *pattern++;
	  if (c != *fn++)
	    return 0;
	}
    }
  return (*fn == '\0');
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
	  if (*line == '\0' || *line == '\n')
	    return 0;
	  line++;
	}
      /* do we have a word delimiter on the left ?? */
      if (IS_ALNUM (line[-1]))
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
get_radix (char const *name)
{
  if (!isdigit (*name))
    return 0;
  if (*name != '0')
    return radix_dec;
  name++;
  if (*name == 'x' || *name == 'X')
    return radix_hex;
  while (*name && *name == '0')
    name++;
  return (*name ? radix_oct : (radix_oct | radix_dec));
}

/* Convert an ascii string number to an integer.  Determine the radix
   before converting.  */

int
stoi (char const *name)
{
  switch (get_radix (name))
    {
    case radix_dec:
      return (dtoi (name));
    case radix_oct:
      return (otoi (&name[1]));
    case radix_hex:
      return (xtoi (&name[2]));
    case radix_dec | radix_oct:
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

struct file_link **
bits_to_flinkv (unsigned char const *bv)
{
  int const reserved_flinkv_slots = 3;
  static struct file_link **flinkv_0;
  struct file_link **flinkv;
  struct file_link **members = members_0;
  struct file_link **end = &members_0[idh.idh_files];

  if (flinkv_0 == 0)
    flinkv_0 = MALLOC (struct file_link *, idh.idh_files + reserved_flinkv_slots + 2);
  flinkv = &flinkv_0[reserved_flinkv_slots];

  for (;;)
    {
      int hits;
      int bit;

      while (*bv == 0)
	{
	  bv++;
	  members += 8;
	  if (members >= end)
	    goto out;
	}
      hits = *bv++;
      for (bit = 1; bit & 0xff; bit <<= 1)
	{
	  if (bit & hits)
	    *flinkv++ = *members;
	  if (++members >= end)
	    goto out;
	}
    }
out:
  *flinkv = 0;
  return &flinkv_0[reserved_flinkv_slots];
}

struct file_link **
tree8_to_flinkv (unsigned char const *hits_tree8)
{
  memset (bits_vec, 0, bits_vec_size);
  return bits_to_flinkv (tree8_to_bits (bits_vec, hits_tree8));
}

#if HAVE_TERMIOS_H

#include <termios.h>
struct termios linemode;
struct termios charmode;
struct termios savemode;
#define GET_TTY_MODES(modes) tcgetattr (0, (modes))
#define SET_TTY_MODES(modes) tcsetattr(0, TCSANOW, (modes))

#else /* not HAVE_TERMIOS_H */

# if HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
# endif

# if HAVE_TERMIO_H

#  include <termio.h>
struct termio linemode;
struct termio charmode;
struct termio savemode;
#define GET_TTY_MODES(modes) ioctl (0, TCGETA, (modes))
#define SET_TTY_MODES(modes) ioctl (0, TCSETA, (modes))

# else /* not HAVE_TERMIO_H */

#  if HAVE_SGTTY_H

#   include <sgtty.h>
struct sgttyb linemode;
struct sgttyb charmode;
struct sgttyb savemode;

#   ifdef TIOCGETP
#define GET_TTY_MODES(modes) ioctl (0, TIOCGETP, (modes))
#define SET_TTY_MODES(modes) ioctl (0, TIOCSETP, (modes))
#   else
#define GET_TTY_MODES(modes) gtty (0, (modes))
#define SET_TTY_MODES(modes) stty (0, (modes))
#   endif

void
savetty (void)
{
#   ifdef TIOCGETP
  ioctl(0, TIOCGETP, &savemode);
#   else
  gtty(0, &savemode);
#   endif
  charmode = linemode = savemode;

  charmode.sg_flags &= ~ECHO;
  charmode.sg_flags |= RAW;

  linemode.sg_flags |= ECHO;
  linemode.sg_flags &= ~RAW;
}

#  endif /* not HAVE_SGTTY_H */
# endif /* not HAVE_TERMIO_H */
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

#endif

#if HAVE_TERMIOS_H || HAVE_TERMIO_H || HAVE_SGTTY_H

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

#endif
