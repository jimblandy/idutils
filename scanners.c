/* scanners.c -- file & directory name manipulations
   Copyright (C) 1986, 1995 Greg McGary
   VHIL portions Copyright (C) 1988 Tom Horsley

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
#include <ctype.h>
#include "strxtra.h"
#include "token.h"
#include "alloc.h"
#include "scanners.h"

extern char const *program_name;

static char const *get_token_VHIL (FILE *input_FILE, int *flags);
static char const *get_token_c (FILE *input_FILE, int *flags);
static void set_args_c (char const *lang_name, int op, char const *arg);
static void set_ctype_c (char const *chars, int type);
static void clear_ctype_c (char const *chars, int type);
static void usage_c (char const *lang_name);

static char const *get_token_asm (FILE *input_FILE, int *flags);
static void set_ctype_asm (char const *chars, int type);
static void clear_ctype_asm (char const *chars, int type);
static void usage_asm (char const *lang_name);
static void set_args_asm (char const *lang_name, int op, char const *arg);

static char const *get_token_text (FILE *input_FILE, int *flags);
static void set_ctype_text (char const *chars, int type);
static void clear_ctype_text (char const *chars, int type);
static void usage_text (char const *lang_name);
static void set_args_text (char const *lang_name, int op, char const *arg);

/****************************************************************************/

struct language
{
  char const *lang_name;
  char const *(*lang_get_token) (FILE *input_FILE, int *flags);
  void (*lang_set_args) (char const *lang_name, int op, char const *arg);
  char const *lang_filter;
  struct language *lang_next;
};

struct suffix
{
  char const *suff_suffix;
  char const *suff_lang_name;
  struct language *suff_language;
  struct suffix *suff_next;
};

static struct suffix *get_suffix_entry (char const *suffix);
static struct language *get_lang_entry (char const *lang_name);
static void usage_scan (void);

struct language languages[] =
{
  /* must be sorted for bsearch(3) */
  { "C", get_token_c, set_args_c, NULL },
  { "TeX", get_token_text, set_args_text, NULL },
  { "VHIL", get_token_VHIL, set_args_c, NULL },
  { "asm", get_token_asm, set_args_asm, NULL },
/*{ "elisp", get_token_elisp, set_args_elisp, NULL },*/
  { "gzip", NULL, NULL, "zcat %s" },
  { "roff", get_token_text, set_args_text, "sed '/^\\.so/d' < %s | deroff" },
  { "text", get_token_text, set_args_text, NULL },
};

/*
	This is a rather incomplete list of default associations
	between suffixes and languages.  You may add more to the
	default list, or you may define them dynamically with the
	`-S<suff>=<lang>' argument to mkid(1) and idx(1).  e.g. to
	associate a `.ada' suffix with the Ada language, use
	`-S.ada=ada'
*/
struct suffix suffixes[] =
{
  { "", "text" },
  { ".1", "roff" },
  { ".2", "roff" },
  { ".3", "roff" },
  { ".4", "roff" },
  { ".5", "roff" },
  { ".6", "roff" },
  { ".7", "roff" },
  { ".8", "roff" },
  { ".C", "C" },
  { ".H", "C" },
  { ".Z", "gzip" },
  { ".c", "C" },
  { ".cc", "C" },
  { ".cpp", "C" },
  { ".cxx", "C" },
  { ".doc", "text" },
/*{ ".el", "elisp" },*/
  { ".gz", "gzip" },
  { ".h", "C" },
  { ".hh", "C" },
  { ".hpp", "C" },
  { ".hxx", "C" },
  { ".l", "C" },
  { ".lex", "C" },
  { ".ltx", "TeX" },
  { ".p", "pas" },
  { ".pas", "pas" },
  { ".s", "asm" },
  { ".S", "asm" },
  { ".tex", "TeX" },
  { ".x", "VHIL" },
  { ".y", "C" },
  { ".yacc", "C" },
  { ".z", "gzip" },
};

void
init_scanners (void)
{
  struct language *lang;
  struct language *lang_N = &languages[(sizeof (languages) / sizeof (languages[0])) - 1];
  struct suffix *suff;
  struct suffix *suff_N = &suffixes[(sizeof (suffixes) / sizeof (suffixes[0])) - 1];
  
  for (lang = languages; lang <= lang_N; ++lang)
    lang->lang_next = lang + 1;
  lang_N->lang_next = NULL;

  for (suff = suffixes; suff <= suff_N; ++suff) {
    lang = get_lang_entry (suff->suff_lang_name);
    if (lang)
      suff->suff_language = lang;
    suff->suff_next = suff + 1;
  }
  suff_N->suff_next = NULL;
}

/* Return a suffix table entry for the given suffix. */
static struct suffix *
get_suffix_entry (char const *suffix)
{
  struct suffix *stp;

  if (suffix == NULL)
    suffix = "";

  for (stp = suffixes; stp; stp = stp->suff_next)
    if (strequ (stp->suff_suffix, suffix))
      return stp;
  return stp;
}

static struct language *
get_lang_entry (char const *lang_name)
{
  struct language *ltp;

  if (lang_name == NULL)
    lang_name = "";

  for (ltp = languages; ltp->lang_next; ltp = ltp->lang_next)
    if (ltp->lang_name == lang_name || strequ (ltp->lang_name, lang_name))
      return ltp;
  return ltp;
}

char const *
get_lang_name (char const *suffix)
{
  struct suffix *stp;

  stp = get_suffix_entry (suffix);
  if (stp->suff_next == NULL)
    return NULL;
  return stp->suff_language->lang_name;
}

char const *
get_filter (char const *suffix)
{
  struct suffix *stp;

  stp = get_suffix_entry (suffix);
  if (stp->suff_next == NULL)
    return NULL;
  return stp->suff_language->lang_filter;
}

char const *(*
get_scanner (char const *lang)
       ) (FILE *input_FILE, int *flags)
{
  struct language *ltp;

  ltp = get_lang_entry (lang);
  if (ltp->lang_next == NULL)
    return NULL;
  return ltp->lang_get_token;
}

void
set_scan_args (int op, char *arg)
{
  struct language *ltp, *ltp2;
  struct suffix *stp;
  char *lhs;
  char *lhs2;
  int count = 0;

  lhs = arg;
  while (isalnum (*arg) || *arg == '.')
    arg++;

  if (strequ (lhs, "?=?"))
    {
      for (stp = suffixes; stp->suff_next; stp = stp->suff_next)
	{
	  printf ("%s%s=%s", (count++ > 0) ? ", " : "", stp->suff_suffix, stp->suff_language->lang_name);
	  if (stp->suff_language->lang_filter)
	    printf (" (%s)", stp->suff_language->lang_filter);
	}
      if (count)
	putchar ('\n');
      return;
    }

  if (strnequ (lhs, "?=", 2))
    {
      lhs += 2;
      ltp = get_lang_entry (lhs);
      if (ltp->lang_next == NULL)
	{
	  printf ("No scanner for language `%s'\n", lhs);
	  return;
	}
      for (stp = suffixes; stp->suff_next; stp = stp->suff_next)
	if (stp->suff_language == ltp)
	  {
	    printf ("%s%s=%s", (count++ > 0) ? ", " : "", stp->suff_suffix, ltp->lang_name);
	    if (stp->suff_language->lang_filter)
	      printf (" (%s)", stp->suff_language->lang_filter);
	  }
      if (count)
	putchar ('\n');
      return;
    }

  if (strequ (arg, "=?"))
    {
      lhs[strlen (lhs) - 2] = '\0';
      stp = get_suffix_entry (lhs);
      if (stp->suff_next == NULL)
	{
	  printf ("No scanner assigned to suffix `%s'\n", lhs);
	  return;
	}
      printf ("%s=%s", stp->suff_suffix, stp->suff_language->lang_name);
      if (stp->suff_language->lang_filter)
	printf (" (%s)", stp->suff_language->lang_filter);
      printf ("\n");
      return;
    }

  if (*arg == '=')
    {
      *arg++ = '\0';

      ltp = get_lang_entry (arg);
      if (ltp->lang_next == NULL)
	{
	  fprintf (stderr, "%s: Language undefined: %s\n", program_name, arg);
	  return;
	}
      stp = get_suffix_entry (lhs);
      if (stp->suff_next == NULL)
	{
	  stp->suff_suffix = lhs;
	  stp->suff_language = ltp;
	  stp->suff_next = CALLOC (struct suffix, 1);
	}
      else if (!strequ (arg, stp->suff_language->lang_name))
	{
	  fprintf (stderr, "%s: Note: `%s=%s' overrides `%s=%s'\n", program_name, lhs, arg, lhs, stp->suff_language->lang_name);
	  stp->suff_language = ltp;
	}
      return;
    }
  else if (*arg == '/')
    {
      *arg++ = '\0';
      ltp = get_lang_entry (lhs);
      if (ltp->lang_next == NULL)
	{
	  ltp->lang_name = lhs;
	  ltp->lang_get_token = get_token_text;
	  ltp->lang_set_args = set_args_text;
	  ltp->lang_filter = NULL;
	  ltp->lang_next = CALLOC (struct language, 1);
	}
      lhs2 = arg;
      arg = strchr (arg, '/');
      if (arg == NULL)
	ltp2 = ltp;
      else
	{
	  *arg++ = '\0';
	  ltp2 = get_lang_entry (lhs2);
	  if (ltp2->lang_next == NULL)
	    {
	      fprintf (stderr, "%s: language %s not defined.\n", program_name, lhs2);
	      ltp2 = ltp;
	    }
	}
      ltp->lang_get_token = ltp2->lang_get_token;
      ltp->lang_set_args = ltp2->lang_set_args;
      if (ltp->lang_filter && (!strequ (arg, ltp->lang_filter)))
	fprintf (stderr, "%s: Note: `%s/%s' overrides `%s/%s'\n", program_name, lhs, arg, lhs, ltp->lang_filter);
      ltp->lang_filter = arg;
      return;
    }

  if (op == '+')
    {
      switch (op = *arg++)
	{
	case '+':
	case '-':
	case '?':
	  break;
	default:
	  usage_scan ();
	}
      for (ltp = languages; ltp->lang_next; ltp = ltp->lang_next)
	(*ltp->lang_set_args) (NULL, op, arg);
      return;
    }

  if (*arg == '-' || *arg == '+' || *arg == '?')
    {
      op = *arg;
      *arg++ = '\0';

      ltp = get_lang_entry (lhs);
      if (ltp->lang_next == NULL)
	{
	  fprintf (stderr, "%s: Language undefined: %s\n", program_name, lhs);
	  return;
	}
      (*ltp->lang_set_args) (lhs, op, arg);
      return;
    }

  usage_scan ();
}

static void
usage_scan (void)
{
  fprintf (stderr, "Usage: %s [-S<suffix>=<lang>] [+S(+|-)<arg>] [-S<lang>(+|-)<arg>] [-S<lang>/<lang>/<filter>]\n", program_name);
  exit (1);
}

/*************** C & C++ ****************************************************/

#define I1	0x0001		/* 1st char of an identifier [a-zA-Z_] */
#define DG	0x0002		/* decimal digit [0-9] */
#define NM	0x0004		/* extra chars in a hex or long number [a-fA-FxXlL] */
#define C1	0x0008		/* C comment introduction char: / */
#define C2	0x0010		/* C comment termination  char: * */
#define Q1	0x0020		/* single quote: ' */
#define Q2	0x0040		/* double quote: " */
#define ES	0x0080		/* escape char: \ */
#define NL	0x0100		/* newline: \n */
#define EF	0x0200		/* EOF */
#define SK	0x0400		/* Make these chars valid for names within strings */
#define VH	0x0800		/* VHIL comment introduction char: # */
#define WS	0x1000		/* White space characters */

/*
	character class membership macros:
*/
#define ISDIGIT(c)	((rct)[c] & (DG))	/* digit */
#define ISNUMBER(c)	((rct)[c] & (DG|NM))	/* legal in a number */
#define ISEOF(c)	((rct)[c] & (EF))	/* EOF */
#define ISID1ST(c)	((rct)[c] & (I1))	/* 1st char of an identifier */
#define ISIDREST(c)	((rct)[c] & (I1|DG))	/* rest of an identifier */
#define ISSTRKEEP(c)	((rct)[c] & (I1|DG|SK))	/* keep contents of string */
#define ISSPACE(c)	((rct)[c] & (WS))	/* white space character */
/*
	The `BORING' classes should be skipped over
	until something interesting comes along...
*/
#define ISBORING(c)	(!((rct)[c] & (EF|NL|I1|DG|Q1|Q2|C1|VH)))	/* fluff */
#define ISCBORING(c)	(!((rct)[c] & (EF|C2)))	/* comment fluff */
#define ISVBORING(c)	(!((rct)[c] & (EF|NL)))	/* vhil comment fluff */
#define ISQ1BORING(c)	(!((rct)[c] & (EF|NL|Q1|ES)))	/* char const fluff */
#define ISQ2BORING(c)	(!((rct)[c] & (EF|NL|Q2|ES)))	/* quoted str fluff */

static unsigned short ctype_c[257] =
{
  EF,
/*      0       1       2       3       4       5       6       7   */
/*    -----   -----   -----   -----   -----   -----   -----   ----- */
/*000*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*010*/ 0,	0,	NL,	0,	0,	0,	0,	0,
/*020*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*030*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*040*/ 0,	0,	Q2,	0,	0,	0,	0,	Q1,
/*050*/ 0,	0,	C2,	0,	0,	0,	0,	C1,
/*060*/ DG,	DG,	DG,	DG,	DG,	DG,	DG,	DG,
/*070*/ DG,	DG,	0,	0,	0,	0,	0,	0,
/*100*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*110*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*120*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*130*/ I1|NM,	I1,	I1,	0,	ES,	0,	0,	I1,
/*140*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*150*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*160*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*170*/ I1|NM,	I1,	I1,	0,	0,	0,	0,	0,
};

static int eat_underscore = 1;
static int scan_VHIL = 0;

static char const *
get_token_VHIL (FILE *input_FILE, int *flags)
{
  if (!scan_VHIL)
    set_args_c ("vhil", '+', "v");
  return get_token_c (input_FILE, flags);
}

/*
	Grab the next identifier the C source
	file opened with the handle `input_FILE'.
	This state machine is built for speed, not elegance.
*/
static char const *
get_token_c (FILE *input_FILE, int *flags)
{
  static char input_buffer[BUFSIZ];
  static int new_line = 1;
  unsigned short *rct = &ctype_c[1];
  int c;
  char *id = input_buffer;

top:
  c = getc (input_FILE);
  if (new_line)
    {
      new_line = 0;
      if (c == '.')
	{
	  /* Auto-recognize vhil code when you see a '.' in column 1.
	     also ignore lines that start with a '.' */
	  if (!scan_VHIL)
	    set_args_c ("vhil", '+', "v");
	  while (ISVBORING (c))
	    c = getc (input_FILE);
	  new_line = 1;
	  goto top;
	}
      if (c != '#')
	goto next;
      c = getc (input_FILE);
      if (scan_VHIL && ISSPACE (c))
	{
	  while (ISVBORING (c))
	    c = getc (input_FILE);
	  new_line = 1;
	  goto top;
	}
      while (ISBORING (c))
	c = getc (input_FILE);
      if (!ISID1ST (c))
	goto next;
      id = input_buffer;
      *id++ = c;
      while (ISIDREST (c = getc (input_FILE)))
	*id++ = c;
      *id = '\0';
      if (strequ (input_buffer, "include"))
	{
	  while (c == ' ' || c == '\t')
	    c = getc (input_FILE);
	  if (c == '\n')
	    {
	      new_line = 1;
	      goto top;
	    }
	  id = input_buffer;
	  if (c == '"')
	    {
	      c = getc (input_FILE);
	      while (c != '\n' && c != EOF && c != '"')
		{
		  *id++ = c;
		  c = getc (input_FILE);
		}
	      *flags = TOK_STRING;
	    }
	  else if (c == '<')
	    {
	      c = getc (input_FILE);
	      while (c != '\n' && c != EOF && c != '>')
		{
		  *id++ = c;
		  c = getc (input_FILE);
		}
	      *flags = TOK_STRING;
	    }
	  else if (ISID1ST (c))
	    {
	      *id++ = c;
	      while (ISIDREST (c = getc (input_FILE)))
		*id++ = c;
	      *flags = TOK_NAME;
	    }
	  else
	    {
	      while (c != '\n' && c != EOF)
		c = getc (input_FILE);
	      new_line = 1;
	      goto top;
	    }
	  while (c != '\n' && c != EOF)
	    c = getc (input_FILE);
	  new_line = 1;
	  *id = '\0';
	  return input_buffer;
	}
      if (strnequ (input_buffer, "if", 2)
	  || strequ (input_buffer, "define")
	  || strequ (input_buffer, "elif")	/* ansi C */
	  || (scan_VHIL && strequ (input_buffer, "elsif"))
	  || strequ (input_buffer, "undef"))
	goto next;
      while ((c != '\n') && (c != EOF))
	c = getc (input_FILE);
      new_line = 1;
      goto top;
    }

next:
  while (ISBORING (c))
    c = getc (input_FILE);

  switch (c)
    {
    case '"':
      id = input_buffer;
      *id++ = c = getc (input_FILE);
      for (;;)
	{
	  while (ISQ2BORING (c))
	    *id++ = c = getc (input_FILE);
	  if (c == '\\')
	    {
	      *id++ = c = getc (input_FILE);
	      continue;
	    }
	  else if (c != '"')
	    goto next;
	  break;
	}
      *--id = '\0';
      id = input_buffer;
      while (ISSTRKEEP (*id))
	id++;
      if (*id || id == input_buffer)
	{
	  c = getc (input_FILE);
	  goto next;
	}
      *flags = TOK_STRING;
      if (eat_underscore && input_buffer[0] == '_' && input_buffer[1])
	return &input_buffer[1];
      else
	return input_buffer;

    case '\'':
      c = getc (input_FILE);
      for (;;)
	{
	  while (ISQ1BORING (c))
	    c = getc (input_FILE);
	  if (c == '\\')
	    {
	      c = getc (input_FILE);
	      continue;
	    }
	  else if (c == '\'')
	    c = getc (input_FILE);
	  goto next;
	}

    case '/':
      c = getc (input_FILE);
      if (c == '/')
	{			/* Cope with C++ comment */
	  while (ISVBORING (c))
	    c = getc (input_FILE);
	  new_line = 1;
	  goto top;
	}
      else if (c != '*')
	goto next;
      c = getc (input_FILE);
      for (;;)
	{
	  while (ISCBORING (c))
	    c = getc (input_FILE);
	  c = getc (input_FILE);
	  if (c == '/')
	    {
	      c = getc (input_FILE);
	      goto next;
	    }
	  else if (ISEOF (c))
	    {
	      new_line = 1;
	      return NULL;
	    }
	}

    case '\n':
      new_line = 1;
      goto top;

    case '#':
      if (!scan_VHIL)
	{
	  /* Auto-recognize vhil when find a # in the middle of a line. */
	  set_args_c ("vhil", '+', "v");
	}
      c = getc (input_FILE);
      while (ISVBORING (c))
	c = getc (input_FILE);
      new_line = 1;
      goto top;
    default:
      if (ISEOF (c))
	{
	  new_line = 1;
	  return NULL;
	}
      id = input_buffer;
      *id++ = c;
      if (ISID1ST (c))
	{
	  *flags = TOK_NAME;
	  while (ISIDREST (c = getc (input_FILE)))
	    *id++ = c;
	}
      else if (ISDIGIT (c))
	{
	  *flags = TOK_NUMBER;
	  while (ISNUMBER (c = getc (input_FILE)))
	    *id++ = c;
	}
      else
	fprintf (stderr, "junk: `\\%3o'", c);
      ungetc (c, input_FILE);
      *id = '\0';
      *flags |= TOK_LITERAL;
      return input_buffer;
    }
}

static void
set_ctype_c (char const *chars, int type)
{
  unsigned short *rct = &ctype_c[1];

  while (*chars)
    rct[*chars++] |= type;
}

static void
clear_ctype_c (char const *chars, int type)
{
  unsigned short *rct = &ctype_c[1];

  while (*chars)
    rct[*chars++] &= ~type;
}

static void
usage_c (char const *lang_name)
{
  fprintf (stderr, "Usage: %s does not accept %s scanner arguments\n", program_name, lang_name);
  exit (1);
}

static char document_c[] = "\
The C scanner arguments take the form -Sc<arg>, where <arg>\n\
is one of the following: (<cc> denotes one or more characters)\n\
  (+|-)u . . . . (Do|Don't) strip a leading `_' from ids in strings.\n\
  (+|-)s<cc> . . Allow <cc> in string ids, and (keep|ignore) those ids.\n\
  -v . . . . . . Skip vhil comments.";

static void
set_args_c (char const *lang_name, int op, char const *arg)
{
  if (op == '?')
    {
      puts (document_c);
      return;
    }
  switch (*arg++)
    {
    case 'u':
      eat_underscore = (op == '+');
      break;
    case 's':
      if (op == '+')
	set_ctype_c (arg, SK);
      else
	clear_ctype_c (arg, SK);
      break;
    case 'v':
      set_ctype_c ("$", I1);
      set_ctype_c ("#", VH);
      set_ctype_c (" \t", WS);
      scan_VHIL = 1;
      break;
    default:
      if (lang_name)
	usage_c (lang_name);
      break;
    }
}

#undef I1
#undef DG
#undef NM
#undef C1
#undef C2
#undef Q1
#undef Q2
#undef ES
#undef NL
#undef EF
#undef SK
#undef VH
#undef WS
#undef ISDIGIT
#undef ISNUMBER
#undef ISEOF
#undef ISID1ST
#undef ISIDREST
#undef ISSTRKEEP
#undef ISSPACE
#undef ISBORING
#undef ISCBORING
#undef ISVBORING
#undef ISQ1BORING
#undef ISQ2BORING

/*************** Assembly ***************************************************/

#define I1	0x01		/* 1st char of an identifier [a-zA-Z_] */
#define NM	0x02		/* digit [0-9a-fA-FxX] */
#define NL	0x04		/* newline: \n */
#define CM	0x08		/* assembler comment char: usually # or | */
#define IG	0x10		/* ignore `identifiers' with these chars in them */
#define C1	0x20		/* C comment introduction char: / */
#define C2	0x40		/* C comment termination  char: * */
#define EF	0x80		/* EOF */

/* Assembly Language character classes */
#define ISID1ST(c)	((rct)[c] & (I1))
#define ISIDREST(c)	((rct)[c] & (I1|NM))
#define ISNUMBER(c)	((rct)[c] & (NM))
#define ISEOF(c)	((rct)[c] & (EF))
#define ISCOMMENT(c)	((rct)[c] & (CM))
#define ISBORING(c)	(!((rct)[c] & (EF|NL|I1|NM|CM|C1)))
#define ISCBORING(c)	(!((rct)[c] & (EF|NL)))
#define ISCCBORING(c)	(!((rct)[c] & (EF|C2)))
#define ISIGNORE(c)	((rct)[c] & (IG))

static unsigned char ctype_asm[257] =
{
  EF,
/*      0       1       2       3       4       5       6       7   */
/*    -----   -----   -----   -----   -----   -----   -----   ----- */
/*000*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*010*/ 0,	0,	NL,	0,	0,	0,	0,	0,
/*020*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*030*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*040*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*050*/ 0,	0,	C2,	0,	0,	0,	0,	C1,
/*060*/ NM,	NM,	NM,	NM,	NM,	NM,	NM,	NM,
/*070*/ NM,	NM,	0,	0,	0,	0,	0,	0,
/*100*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*110*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*120*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*130*/ I1|NM,	I1,	I1,	0,	0,	0,	0,	I1,
/*140*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*150*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*160*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*170*/ I1|NM,	I1,	I1,	0,	0,	0,	0,	0,

};

static int cpp_on_asm = 1;

/*
	Grab the next identifier the assembly language
	source file opened with the handle `input_FILE'.
	This state machine is built for speed, not elegance.
*/
static char const *
get_token_asm (FILE *input_FILE, int *flags)
{
  static char input_buffer[BUFSIZ];
  unsigned char *rct = &ctype_asm[1];
  int c;
  char *id = input_buffer;
  static int new_line = 1;

top:
  c = getc (input_FILE);
  if (cpp_on_asm > 0 && new_line)
    {
      new_line = 0;
      if (c != '#')
	goto next;
      while (ISBORING (c))
	c = getc (input_FILE);
      if (!ISID1ST (c))
	goto next;
      id = input_buffer;
      *id++ = c;
      while (ISIDREST (c = getc (input_FILE)))
	*id++ = c;
      *id = '\0';
      if (strequ (input_buffer, "include"))
	{
	  while (c != '"' && c != '<')
	    c = getc (input_FILE);
	  id = input_buffer;
	  *id++ = c = getc (input_FILE);
	  while ((c = getc (input_FILE)) != '"' && c != '>')
	    *id++ = c;
	  *id = '\0';
	  *flags = TOK_STRING;
	  return input_buffer;
	}
      if (strnequ (input_buffer, "if", 2)
	  || strequ (input_buffer, "define")
	  || strequ (input_buffer, "undef"))
	goto next;
      while (c != '\n')
	c = getc (input_FILE);
      new_line = 1;
      goto top;
    }

next:
  while (ISBORING (c))
    c = getc (input_FILE);

  if (ISCOMMENT (c))
    {
      while (ISCBORING (c))
	c = getc (input_FILE);
      new_line = 1;
    }

  if (ISEOF (c))
    {
      new_line = 1;
      return NULL;
    }

  if (c == '\n')
    {
      new_line = 1;
      goto top;
    }

  if (c == '/')
    {
      if ((c = getc (input_FILE)) != '*')
	goto next;
      c = getc (input_FILE);
      for (;;)
	{
	  while (ISCCBORING (c))
	    c = getc (input_FILE);
	  c = getc (input_FILE);
	  if (c == '/')
	    {
	      c = getc (input_FILE);
	      break;
	    }
	  else if (ISEOF (c))
	    {
	      new_line = 1;
	      return NULL;
	    }
	}
      goto next;
    }

  id = input_buffer;
  if (eat_underscore && c == '_' && !ISID1ST (c = getc (input_FILE)))
    {
      ungetc (c, input_FILE);
      return "_";
    }
  *id++ = c;
  if (ISID1ST (c))
    {
      *flags = TOK_NAME;
      while (ISIDREST (c = getc (input_FILE)))
	*id++ = c;
    }
  else if (ISNUMBER (c))
    {
      *flags = TOK_NUMBER;
      while (ISNUMBER (c = getc (input_FILE)))
	*id++ = c;
    }
  else
    {
      if (isprint (c))
	fprintf (stderr, "junk: `%c'", c);
      else
	fprintf (stderr, "junk: `\\%03o'", c);
      goto next;
    }

  *id = '\0';
  for (id = input_buffer; *id; id++)
    if (ISIGNORE (*id))
      goto next;
  ungetc (c, input_FILE);
  *flags |= TOK_LITERAL;
  return input_buffer;
}

static void
set_ctype_asm (char const *chars, int type)
{
  unsigned char *rct = &ctype_asm[1];

  while (*chars)
    rct[*chars++] |= type;
}

static void
clear_ctype_asm (char const *chars, int type)
{
  unsigned char *rct = &ctype_asm[1];

  while (*chars)
    rct[*chars++] &= ~type;
}

static void
usage_asm (char const *lang_name)
{
  fprintf (stderr, "Usage: %s -S%s([-c<cc>] [-u] [(+|-)a<cc>] [(+|-)p] [(+|-)C])\n", program_name, lang_name);
  exit (1);
}

static char document_asm[] = "\
The Assembler scanner arguments take the form -Sasm<arg>, where\n\
<arg> is one of the following: (<cc> denotes one or more characters)\n\
  -c<cc> . . . . <cc> introduce(s) a comment until end-of-line.\n\
  (+|-)u . . . . (Do|Don't) strip a leading `_' from ids.\n\
  (+|-)a<cc> . . Allow <cc> in ids, and (keep|ignore) those ids.\n\
  (+|-)p . . . . (Do|Don't) handle C-preprocessor directives.\n\
  (+|-)C . . . . (Do|Don't) handle C-style comments. (/* */)";

static void
set_args_asm (char const *lang_name, int op, char const *arg)
{
  if (op == '?')
    {
      puts (document_asm);
      return;
    }
  switch (*arg++)
    {
    case 'a':
      set_ctype_asm (arg, I1 | ((op == '-') ? IG : 0));
      break;
    case 'c':
      set_ctype_asm (arg, CM);
      break;
    case 'u':
      eat_underscore = (op == '+');
      break;
    case 'p':
      cpp_on_asm = (op == '+');
      break;
    case 'C':
      if (op == '+')
	{
	  set_ctype_asm ("/", C1);
	  set_ctype_asm ("*", C2);
	}
      else
	{
	  clear_ctype_asm ("/", C1);
	  clear_ctype_asm ("*", C2);
	}
      break;
    default:
      if (lang_name)
	usage_asm (lang_name);
      break;
    }
}

#undef I1
#undef NM
#undef NL
#undef CM
#undef IG
#undef C1
#undef C2
#undef EF
#undef ISID1ST
#undef ISIDREST
#undef ISNUMBER
#undef ISEOF
#undef ISCOMMENT
#undef ISBORING
#undef ISCBORING
#undef ISCCBORING
#undef ISIGNORE

/*************** Text *******************************************************/

#define I1	0x01		/* 1st char of an identifier [a-zA-Z_] */
#define NM	0x02		/* digit [0-9a-fA-FxX] */
#define SQ	0x04		/* squeeze these out (.,',-) */
#define EF	0x80		/* EOF */

/* Text character classes */
#define ISID1ST(c)	((rct)[c] & (I1))
#define ISIDREST(c)	((rct)[c] & (I1|NM|SQ))
#define ISNUMBER(c)	((rct)[c] & (NM))
#define ISEOF(c)	((rct)[c] & (EF))
#define ISBORING(c)	(!((rct)[c] & (I1|NM|EF)))
#define ISIDSQUEEZE(c)	((rct)[c] & (SQ))

static unsigned char ctype_text[257] =
{
  EF,
/*      0       1       2       3       4       5       6       7   */
/*    -----   -----   -----   -----   -----   -----   -----   ----- */
/*000*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*010*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*020*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*030*/ 0,	0,	0,	0,	0,	0,	0,	0,
/*040*/ 0,	0,	0,	0,	0,	0,	0,	SQ,
/*050*/ 0,	0,	0,	0,	0,	SQ,	SQ,	0,
/*060*/ NM,	NM,	NM,	NM,	NM,	NM,	NM,	NM,
/*070*/ NM,	NM,	0,	0,	0,	0,	0,	0,
/*100*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*110*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*120*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*130*/ I1|NM,	I1,	I1,	0,	0,	0,	0,	I1,
/*140*/ 0,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1|NM,	I1,
/*150*/ I1,	I1,	I1,	I1,	I1|NM,	I1,	I1,	I1,
/*160*/ I1,	I1,	I1,	I1,	I1,	I1,	I1,	I1,
/*170*/ I1|NM,	I1,	I1,	0,	0,	0,	0,	0,
};

/*
        Grab the next identifier the text source file opened with the
	handle `input_FILE'.  This state machine is built for speed, not
	elegance.
*/
static char const *
get_token_text (FILE *input_FILE, int *flags)
{
  static char input_buffer[BUFSIZ];
  unsigned char *rct = &ctype_text[1];
  int c;
  char *id = input_buffer;

top:
  c = getc (input_FILE);
  while (ISBORING (c))
    c = getc (input_FILE);
  if (ISEOF (c))
    return NULL;
  id = input_buffer;
  *id++ = c;
  if (ISID1ST (c))
    {
      *flags = TOK_NAME;
      while (ISIDREST (c = getc (input_FILE)))
	if (!ISIDSQUEEZE (c))
	  *id++ = c;
    }
  else if (ISNUMBER (c))
    {
      *flags = TOK_NUMBER;
      while (ISNUMBER (c = getc (input_FILE)))
	*id++ = c;
    }
  else
    {
      if (isprint (c))
	fprintf (stderr, "junk: `%c'", c);
      else
	fprintf (stderr, "junk: `\\%03o'", c);
      goto top;
    }

  *id = '\0';
  ungetc (c, input_FILE);
  *flags |= TOK_LITERAL;
  return input_buffer;
}

static void
set_ctype_text (char const *chars, int type)
{
  unsigned char *rct = &ctype_text[1];

  while (*chars)
    rct[*chars++] |= type;
}

static void
clear_ctype_text (char const *chars, int type)
{
  unsigned char *rct = &ctype_text[1];

  while (*chars)
    rct[*chars++] &= ~type;
}

static void
usage_text (char const *lang_name)
{
  fprintf (stderr, "Usage: %s -S%s([(+|-)a<cc>] [(+|-)s<cc>]\n", program_name, lang_name);
  exit (1);
}

static char document_text[] = "\
The Text scanner arguments take the form -Stext<arg>, where\n\
<arg> is one of the following: (<cc> denotes one or more characters)\n\
  (+|-)a<cc> . . Include (or exculde) <cc> in ids.\n\
  (+|-)s<cc> . . Squeeze (or don't squeeze) <cc> out of ids.";

static void
set_args_text (char const *lang_name, int op, char const *arg)
{
  if (op == '?')
    {
      puts (document_text);
      return;
    }
  switch (*arg++)
    {
    case 'a':
      if (op == '+')
	set_ctype_text (arg, I1);
      else
	clear_ctype_text (arg, I1);
      break;
    case 's':
      if (op == '+')
	set_ctype_text (arg, SQ);
      else
	clear_ctype_text (arg, SQ);
      break;
    default:
      if (lang_name)
	usage_text (lang_name);
      break;
    }
}

#undef I1
#undef NM
#undef SQ
#undef EF
#undef ISID1ST
#undef ISIDREST
#undef ISNUMBER
#undef ISEOF
#undef ISBORING
#undef ISIDSQUEEZE
