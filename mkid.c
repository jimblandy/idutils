/* mkid.c -- build an identifer database
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <config.h>
#include "strxtra.h"
#include "alloc.h"
#include "idfile.h"
#include "token.h"
#include "bitops.h"
#include "misc.h"
#include "filenames.h"
#include "hash.h"
#include "scanners.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

struct summary
{
  struct token **sum_tokens;
  unsigned char const *sum_hits;
  struct summary *sum_parent;
  union {
    struct summary *u_kids[8];	/* when sum_level > 0 */
    struct idarg *u_files[8];	/* when sum_level == 0 */
  } sum_u;
#define sum_kids sum_u.u_kids
#define sum_files sum_u.u_files
  unsigned long sum_tokens_size;
  unsigned long sum_hits_count;
  int sum_free_index;
  int sum_level;
};

#define MAX_LEVELS 5		/* log_8 of the max # of files: log_8(32768) == 5 */

struct token
{
  unsigned short tok_count;
  unsigned char tok_flags;
  unsigned char tok_hits[MAX_LEVELS];
  char tok_name[1];
};

char *bitsset __P((char *s1, char const *s2, int n));
char *bitsclr __P((char *s1, char const *s2, int n));
char *bitsand __P((char *s1, char const *s2, int n));
char *bitsxor __P((char *s1, char const *s2, int n));
int bitstst __P((char const *s1, char const *s2, int n));
int bitsany __P((char const *s, int n));
struct token *make_token __P((char const *name, int));
void scan_1_file __P((char const *(*get_token) (FILE*, int*), FILE *source_FILE));
struct idarg *parse_idargs __P((int argc, char **argv));
struct idarg *parse_idargs_from_FILE __P((FILE *arg_FILE, struct idarg *idarg));
void scan_files __P((struct idarg *idarg));
void report_statistics __P((void));

unsigned long token_hash_1 __P((void const *key));
unsigned long token_hash_2 __P((void const *key));
int token_hash_cmp __P((void const *x, void const *y));

void write_idfile __P((char const *id_file, struct idarg *idargs));
void bump_current_hits_signature __P((void));
void init_hits_signature __P((int i));
int bit_to_index __P((int bit));
int token_qsort_cmp __P((void const *x, void const *y));
void free_summary_tokens __P((void));
void summarize __P((void));
void assert_hits __P((struct summary *summary));
void write_hits __P((FILE *fp, struct summary *summary, unsigned char const *tail_hits));
void sign_token __P((struct token *token));
void add_token_to_summary __P((struct summary *summary, struct token *token));
void init_summary __P((void));
struct summary *make_sibling_summary __P((struct summary *summary));
int count_vec_size __P((struct summary *summary, unsigned char const *tail_hits));
int count_buf_size __P((struct summary *summary, unsigned char const *tail_hits));
void usage __P((void));

struct hash_table token_table;
struct hash_table file_table;

/* Miscellaneous statistics */
long input_chars;
long name_tokens;
long number_tokens;
long string_tokens;
long literal_tokens;
long comment_tokens;
long occurrences;
long heap_size;
long hits_length = 0;
long tokens_length = 0;
long output_length = 0;

int verbose_flag = 0;
int statistics_flag = 1;

int args_count = 0;		/* # of args to save */
int scan_count = 0;		/* # of files to scan */
int file_name_count = 0;	/* # of files in database */
int levels = 0;			/* ceil(log(8)) of file_name_count */

unsigned char current_hits_signature[MAX_LEVELS];
#define INIT_TOKENS_SIZE(level) (1 << ((level) + 13))
struct summary *summary_root;
struct summary *summary_leaf;

char PWD_buf[BUFSIZ];		/* The current working directory */
char absolute_idfile_name[BUFSIZ]; /* The absolute name of the database */
char const *id_file_name = IDFILE;

char const *program_name;

void
usage (void)
{
  fprintf (stderr, "\
Usage: %s [-v] [-f<idfile>] [(+|-)l[<lang>]] [(+|-)S<scanarg>] [-a<argfile>] [-] [files...]\n\
	-v	 Verbose: print reports of progress\n\
	-a<file> Open file for arguments\n\
	-	 Read newline-separated args from stdin\n\
	-l<lang> Force files to be scanned as <lang> until +l<lang>\n\
	-S<lang>-<arg> Pass arg to <lang> scanner\n\
	-S.<suffix>=<lang> Scan files with .<suffix> as <lang>\n\
	-S<lang>? Print usage documentation for <lang>\n\
\n\
Version %s",
	   program_name, VERSION);
#ifdef __DATE__
  fprintf (stderr, "; Made %s %s", __DATE__, __TIME__);
#endif
  fputc ('\n', stderr);
  exit (1);
}

void *sbrk ();

int
main (int argc, char **argv)
{
  struct idarg *idarg_0;
  char const *sbrk0;

  program_name = basename ((argc--, *argv++));
  init_scanners ();

  idarg_0 = parse_idargs (argc, argv);
  if (idarg_0 == NULL)
    {
      fprintf (stderr, "Nothing to do...\n");
      return 0;
    }

  sbrk0 = (char const *) sbrk (0);
  hash_init (&token_table, scan_count * 64, token_hash_1, token_hash_2, token_hash_cmp);

  get_PWD (PWD_buf);
  strcpy (absolute_idfile_name, span_file_name (PWD_buf, id_file_name));
  if (access (id_file_name, 06) < 0
      && (errno != ENOENT || access (dirname (id_file_name), 06) < 0))
    {
      filerr ("modify", id_file_name);
      return 1;
    }
  
  init_hits_signature (0);
  init_summary ();
  
  scan_files (idarg_0);

  if (token_table.ht_fill == 0)
    return 0;

  free_summary_tokens ();
  free (token_table.ht_vec);

  write_idfile (id_file_name, idarg_0);
  heap_size = (char const *) sbrk (0) - sbrk0;

  if (statistics_flag)
    report_statistics ();
  return 0;
}

void
scan_files (struct idarg *idarg)
{
  int keep_lang = 0;

  for ( ; idarg->ida_next; idarg = idarg->ida_next)
    {
      char const *(*scanner) __P((FILE*, int*));
      FILE *source_FILE;
      char *arg = idarg->ida_arg;
      char const *lang_name = NULL;
      char const *suff;
      char const *filter;

      if (idarg->ida_index < 0)
	{
	  int op = *arg++;
	  switch (*arg++)
	    {
	    case 'l':
	      if (*arg == '\0')
		{
		  keep_lang = 0;
		  lang_name = NULL;
		  break;
		}
	      if (op == '+')
		keep_lang = 1;
	      lang_name = arg;
	      break;
	    case 'S':
	      set_scan_args (op, strdup (arg));
	      break;
	    default:
	      usage ();
	    }
	  continue;
	}
      if (!(idarg->ida_flags & IDA_SCAN_ME))
	goto skip;

      suff = strrchr (arg, '.');
      if (lang_name == NULL)
	{
	  if (suff == NULL)
	    suff = "";
	  lang_name = get_lang_name (suff);
	  if (lang_name == NULL)
	    lang_name = get_lang_name ("");
	  if (lang_name == NULL)
	    {
	      fprintf (stderr, "%s: No language assigned to suffix: `%s'\n", program_name, suff);
	      goto skip;
	    }
	}
      scanner = get_scanner (lang_name);
      if (scanner == NULL)
	{
	  fprintf (stderr, "%s: No scanner for language: `%s'\n", program_name, lang_name);
	  goto skip;
	}
      filter = get_filter (suff);
      source_FILE = open_source_FILE (arg, filter);
      if (source_FILE == NULL)
	goto skip;
      if (verbose_flag)
	{
	  printf ("%s: ", lang_name);
	  printf (filter ? filter : "%s", arg);
	  fflush (stdout);
	}
      scan_1_file (scanner, source_FILE);
      if (verbose_flag)
	putchar ('\n');
      close_source_FILE (source_FILE, filter);
    skip:
      if (!keep_lang)
	lang_name = NULL;
      if (idarg->ida_index < file_name_count)
	{
	  if (current_hits_signature[0] & 0x80)
	    summarize ();
	  bump_current_hits_signature ();
	}
    }
}

void
report_statistics (void)
{
  printf ("Name=%ld, ", name_tokens);
  printf ("Number=%ld, ", number_tokens);
  printf ("String=%ld, ", string_tokens);
  printf ("Literal=%ld, ", literal_tokens);
  printf ("Comment=%ld\n", comment_tokens);

  printf ("Files=%d, ", scan_count);
  printf ("Tokens=%ld, ", occurrences);
  printf ("Bytes=%ld Kb, ", input_chars / 1024);
  printf ("Heap=%ld Kb, ", heap_size / 1024);
  printf ("Output=%ld (%ld tok, %ld hit)\n", output_length, tokens_length, hits_length);

  printf ("Load=%ld/%ld=%.2f, ", token_table.ht_fill, token_table.ht_size,
	  (double) token_table.ht_fill / (double) token_table.ht_size);
  printf ("Rehash=%d, ", token_table.ht_rehashes);
  printf ("Probes=%ld/%ld=%.2f, ", token_table.ht_probes, token_table.ht_lookups,
	  (double) token_table.ht_probes / (double) token_table.ht_lookups);
  printf ("Freq=%ld/%ld=%.2f\n", occurrences, token_table.ht_fill,
	  (double) occurrences / (double) token_table.ht_fill);
}

struct idarg *
parse_idargs (int argc, char **argv)
{
  struct idarg *idarg;
  struct idarg *idarg_0;
  char *arg;
  int op;
  FILE *arg_FILE = NULL; 
  int args_from = 0;
  enum {
      AF_CMDLINE = 0x1,		/* file args came on command line */
      AF_FILE = 0x2,		/* file args came from a file (-f<file>) */
      AF_USAGE = 0x8
  };				/* no file args necessary: usage query */

  idarg = idarg_0 = CALLOC (struct idarg, 1);

  /* Process some arguments, and snarf-up some others for processing
     later.  */
  while (argc)
    {
      arg = (argc--, *argv++);
      if (*arg != '-' && *arg != '+')
	{
	  /* arguments are from command line (not pipe) */
	  args_from |= AF_CMDLINE;
	  idarg->ida_arg = arg;
	  idarg->ida_flags = IDA_SCAN_ME;
	  idarg->ida_index = file_name_count++;
	  scan_count++;
	  idarg = (idarg->ida_next = CALLOC (struct idarg, 1));

	  continue;
	}
      op = *arg++;
      switch (*arg++)
	{
	case '\0':
	  args_from |= AF_FILE;
	  idarg = parse_idargs_from_FILE (stdin, idarg);
	  break;
	case 'a':
	  arg_FILE = fopen (arg, "r");
	  if (arg_FILE == NULL)
	    filerr ("open", arg);
	  else
	    {
	      args_from |= AF_FILE;
	      idarg = parse_idargs_from_FILE (arg_FILE, idarg);
	    }
	  break;
	case 'f':
	  id_file_name = arg;
	  break;
	case 'v':
	  verbose_flag = 1;
	  break;
	case 'S':
	  if (strchr (&arg[-2], '?'))
	    {
	      set_scan_args (op, arg);
	      args_from |= AF_USAGE;
	    }
	  /*FALLTHROUGH */
	case 'l':
	  idarg->ida_arg = &arg[-2];
	  idarg->ida_index = -1;
	  idarg = (idarg->ida_next = CALLOC (struct idarg, 1));

	  args_count++;
	  break;
	default:
	  usage ();
	}
    }

  if (args_from & AF_USAGE)
    exit (0);
  /* File args should only come from one place.  Ding the user if
     arguments came from multiple places, or if none were supplied at
     all.  */
  switch (args_from)
    {
    case AF_CMDLINE:
    case AF_FILE:
      if (file_name_count > 0)
	break;
      /*FALLTHROUGH */
    case 0:
      fprintf (stderr, "%s: Use -u, -f<file>, or cmd-line for file args!\n", program_name);
      usage ();
    default:
      fprintf (stderr, "%s: Use only one of: -u, -f<file>, or cmd-line for file args!\n", program_name);
      usage ();
    }

  if (scan_count == 0)
    return NULL;

  return idarg_0;
}


/* Cons up a list of idarg as supplied in a file. */
struct idarg *
parse_idargs_from_FILE (FILE *arg_FILE, struct idarg *idarg)
{
  int file_count;
  char buf[BUFSIZ];
  char *arg;

  file_count = 0;
  while (fgets (buf, sizeof (buf), arg_FILE))
    {
      idarg->ida_arg = arg = strndup (buf, strlen (buf) - 1);
      if (*arg == '+' || *arg == '-')
	idarg->ida_index = -1;
      else
	{
	  idarg->ida_flags = IDA_SCAN_ME;
	  idarg->ida_index = file_name_count++;
	  scan_count++;
	}
      idarg = idarg->ida_next = CALLOC (struct idarg, 1);
    }
  return idarg;
}

void
scan_1_file (get_token_t get_token, FILE *source_FILE)
{
  struct stat stat_buf;
  struct token **slot;
  char const *key;
  int bytes = 0;
  int total_tokens = 0;
  int new_tokens = 0;
  int distinct_tokens = 0;
  int flags;
  struct token *token;

  if (fstat (fileno (source_FILE), &stat_buf) == 0)
    {
      bytes = stat_buf.st_size;
      input_chars += bytes;
    }

  while ((key = (*get_token) (source_FILE, &flags)) != NULL)
    {
      if (*key == '\0')
	continue;
      total_tokens++;
      slot = (struct token **) hash_lookup (&token_table, key - offsetof (struct token, tok_name));
      token = *slot;
      if (token)
	{
	  token->tok_flags |= flags;
	  if (token->tok_count < USHRT_MAX)
	    token->tok_count++;
	  if (!(token->tok_hits[0] & current_hits_signature[0]))
	    {
	      sign_token (token);
	      distinct_tokens++;
	    }
	} else {
	  *slot = token = make_token (key, flags);
	  sign_token (token);
	  distinct_tokens++;
	  new_tokens++;
	  if (token_table.ht_fill++ >= token_table.ht_capacity)
	    rehash (&token_table);
	}
    }
  if (verbose_flag)
    {
      printf ("  uniq=%d/%d", distinct_tokens, total_tokens);
      if (total_tokens != 0)
	printf ("=%.2f", (double) distinct_tokens / (double) total_tokens);
      printf (", new=%d/%d", new_tokens, distinct_tokens);
      if (distinct_tokens != 0)
	printf ("=%.2f", (double) new_tokens / (double) distinct_tokens);
    }
}

/* As the database is written, may need to adjust the file names.  If
   we are generating the ID file in a remote directory, then adjust
   the file names to be relative to the location of the ID database.

   (This would be a common useage if you want to make a database for a
   directory which you have no write access to, so you cannot create
   the ID file.)  */
void 
write_idfile (char const *file_name, struct idarg *idarg)
{
  struct token **tokens;
  int i;
  FILE *id_FILE;
  struct idhead idh;
  int fixup_names;
  char *lsl;
  int buf_size;
  int vec_size;
  int tok_size;
  int max_buf_size = 0;
  int max_vec_size = 0;

  if (verbose_flag)
    printf ("Sorting tokens...\n");
  assert (summary_root->sum_hits_count == token_table.ht_fill);
  tokens = REALLOC (summary_root->sum_tokens, struct token *, token_table.ht_fill);
  qsort (tokens, token_table.ht_fill, sizeof (struct token *), token_qsort_cmp);

  if (verbose_flag)
    printf ("Writing `%s'...\n", file_name);
  lsl = strrchr (relative_file_name (PWD_buf, absolute_idfile_name), '/');
  if (lsl == NULL)
    {
      /* The database is in the cwd, don't adjust the names */
      fixup_names = 0;
    }
  else
    {
      /* The database is not in cwd, adjust names so they are relative
	 to the location of the database, make absolute_idfile_name just be the
	 directory path to ID.  */
      fixup_names = 1;
      *(lsl + 1) = '\0';
    }
  id_FILE = fopen (file_name, "w+b");
  if (id_FILE == NULL)
    {
      filerr ("create", file_name);
      exit (1);
    }
  idh.idh_magic[0] = IDH_MAGIC_0;
  idh.idh_magic[1] = IDH_MAGIC_1;
  idh.idh_version = IDH_VERSION;
  idh.idh_flags = IDH_COUNTS;

  /* write out the list of pathnames */
  fseek (id_FILE, sizeof_idhead (), 0);
  idh.idh_args_offset = ftell (id_FILE);
  for ( ; idarg->ida_next; idarg = idarg->ida_next)
    {
      if (*idarg->ida_arg != '-' && fixup_names)
	fputs (relative_file_name (absolute_idfile_name, span_file_name (PWD_buf, idarg->ida_arg)), id_FILE);
      else
	fputs (idarg->ida_arg, id_FILE);
      putc ('\0', id_FILE);
    }
  idh.idh_files = file_name_count;

  /* write out the list of identifiers */

  putc ('\0', id_FILE);
  putc ('\0', id_FILE);
  idh.idh_tokens_offset = ftell (id_FILE);
  
  for (i = 0; i < token_table.ht_fill; i++, tokens++)
    {
      struct token *token = *tokens;
      occurrences += token->tok_count;
      if (token->tok_flags & TOK_NUMBER)
	number_tokens++;
      if (token->tok_flags & TOK_NAME)
	name_tokens++;
      if (token->tok_flags & TOK_STRING)
	string_tokens++;
      if (token->tok_flags & TOK_LITERAL)
	literal_tokens++;
      if (token->tok_flags & TOK_COMMENT)
	comment_tokens++;

      fputs (token->tok_name, id_FILE);
      putc ('\0', id_FILE);
      if (token->tok_count > 0xff)
	token->tok_flags |= TOK_SHORT_COUNT;
      putc (token->tok_flags, id_FILE);
      putc (token->tok_count & 0xff, id_FILE);
      if (token->tok_flags & TOK_SHORT_COUNT)
	putc (token->tok_count >> 8, id_FILE);

      vec_size = count_vec_size (summary_root, token->tok_hits + levels);
      buf_size = count_buf_size (summary_root, token->tok_hits + levels);
      hits_length += buf_size;
      tok_size = strlen (token->tok_name) + 1;
      tokens_length += tok_size;
      buf_size += tok_size + sizeof (token->tok_flags) + sizeof (token->tok_count) + 2;
      if (buf_size > max_buf_size)
	  max_buf_size = buf_size;
      if (vec_size > max_vec_size)
	  max_vec_size = vec_size;

      write_hits (id_FILE, summary_root, token->tok_hits + levels);
      putc ('\0', id_FILE);
      putc ('\0', id_FILE);
    }
  assert_hits (summary_root);
  idh.idh_tokens = token_table.ht_fill;
  output_length = ftell (id_FILE);
  idh.idh_end_offset = output_length - 2;
  idh.idh_buf_size = max_buf_size;
  idh.idh_vec_size = max_vec_size;

  write_idhead (id_FILE, &idh);
  fclose (id_FILE);
}

unsigned long
token_hash_1 (void const *key)
{
  return_STRING_HASH_1 (((struct token const *) key)->tok_name);
}

unsigned long
token_hash_2 (void const *key)
{
  return_STRING_HASH_2 (((struct token const *) key)->tok_name);
}

int
token_hash_cmp (void const *x, void const *y)
{
  return_STRING_COMPARE (((struct token const *) x)->tok_name,
			 ((struct token const *) y)->tok_name);
}

int
token_qsort_cmp (void const *x, void const *y)
{
  return_STRING_COMPARE ((*(struct token const *const *) x)->tok_name,
			 (*(struct token const *const *) y)->tok_name);
}

struct token *
make_token (char const *name, int flags)
{
  struct token *token = (struct token *) malloc (sizeof (struct token) + strlen (name));

  if (!token)
    {
      fprintf (stderr, "malloc failure! \n");
      exit (1);
    }
  token->tok_count = 1;
  token->tok_flags = flags;
  memset (token->tok_hits, 0, sizeof (token->tok_hits));
  strcpy (token->tok_name, name);

  return token;
}

/* ///////////// summary stuff //////////////////////////////////////////// */

void
bump_current_hits_signature (void)
{
  unsigned char *hits = current_hits_signature;
  while (*hits & 0x80)
    *hits++ = 1;
  *hits <<= 1;
}

void
init_hits_signature (int i)
{
  unsigned char *hits = current_hits_signature;
  unsigned char const *end = &current_hits_signature[MAX_LEVELS];
  while (hits < end)
    {
      *hits = 1 << (i & 7);
      i >>= 3;
      hits++;
    }
}

int
bit_to_index (int bit)
{
  int i = 0;
  while (bit >>= 1)
    i++;
  return i;
}

void
free_summary_tokens (void)
{
  struct summary *summary = summary_leaf;
  while (summary != summary_root)
    {
      free (summary->sum_tokens);
      summary = summary->sum_parent;
    }
}

void
summarize (void)
{
  unsigned char const *hits_sig = current_hits_signature;
  struct summary *summary = summary_leaf;

  do
    {
      unsigned long count = summary->sum_hits_count;
      unsigned char *hits = MALLOC (unsigned char, count + 1);
      unsigned int level = summary->sum_level;
      struct token **tokens = summary->sum_tokens;
      unsigned long init_size = INIT_TOKENS_SIZE (summary->sum_level);

      if (verbose_flag)
	{
	  char const *fmt;
	  if (count < init_size / 2)
	    fmt = "level %d: %ld < %ld/2\n";
	  else if (count > init_size * 2)
	    fmt = "level %d: %ld > %ld*2\n";
	  else if (count < init_size)
	    fmt = "level %d: %ld < %ld\n";
	  else if (count > init_size)
	    fmt = "level %d: %ld > %ld\n";
	  else
	    fmt = "level %d: %ld == %ld\n";
	  printf (fmt, summary->sum_level, count, init_size);
	}

      qsort (tokens, count, sizeof (struct token *), token_qsort_cmp);
      summary->sum_hits = hits;
      while (count--)
	{
	  unsigned char *hit = &(*tokens++)->tok_hits[level];
	  *hits++ = *hit;
	  *hit = 0;
	}
      *hits++ = 0;
      if (summary->sum_parent)
	{
	  free (summary->sum_tokens);
	  summary->sum_tokens = 0;
	}
      summary = summary->sum_parent;
    }
  while (*++hits_sig & 0x80);
  summary_leaf = make_sibling_summary (summary_leaf);
}

void
init_summary (void)
{
  unsigned long size = INIT_TOKENS_SIZE (0);
  summary_root = summary_leaf = CALLOC (struct summary, 1);
  summary_root->sum_tokens_size = size;
  summary_root->sum_tokens = MALLOC (struct token *, size);
}

struct summary *
make_sibling_summary (struct summary *summary)
{
  struct summary *parent = summary->sum_parent;
  unsigned long size;

  if (parent == NULL)
    {
      levels++;
      summary_root = summary->sum_parent = parent = CALLOC (struct summary, 1);
      parent->sum_level = levels;
      parent->sum_kids[0] = summary;
      parent->sum_hits_count = summary->sum_hits_count;
      parent->sum_free_index = 1;
      size = INIT_TOKENS_SIZE (levels);
      if (summary->sum_tokens_size >= size)
	{
	  parent->sum_tokens_size = summary->sum_tokens_size;
	  parent->sum_tokens = summary->sum_tokens;
	}
      else
	{
	  parent->sum_tokens_size = size;
	  parent->sum_tokens = REALLOC (summary->sum_tokens, struct token *, size);
	}
      summary->sum_tokens = 0;
    }
  if (parent->sum_free_index == 8)
    parent = make_sibling_summary (parent);
  summary = CALLOC (struct summary, 1);
  summary->sum_level = parent->sum_level - 1;
  parent->sum_kids[parent->sum_free_index++] = summary;
  summary->sum_parent = parent;
  size = INIT_TOKENS_SIZE (summary->sum_level);
  summary->sum_tokens_size = size;
  summary->sum_tokens = MALLOC (struct token *, size);
  return summary;
}

int
count_vec_size (struct summary *summary, unsigned char const *tail_hits)
{
  struct summary **kids;
  unsigned int hits = (summary->sum_hits ? *summary->sum_hits : *tail_hits);

  kids = summary->sum_kids;
  if (*kids == NULL)
    {
      static char bits_per_nybble[] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
      return bits_per_nybble[hits & 0xf] + bits_per_nybble[hits >> 4];
    }
  else
    {
      int bit;
      int count = 0;
      --tail_hits;
      for (bit = 1; bit & 0xff; bit <<= 1, ++kids)
	if (bit & hits)
	  count += count_vec_size (*kids, tail_hits);
      return count;
    }
}

int
count_buf_size (struct summary *summary, unsigned char const *tail_hits)
{
  struct summary **kids;
  unsigned int hits = (summary->sum_hits ? *summary->sum_hits : *tail_hits);

  kids = summary->sum_kids;
  if (*kids == NULL)
    return 1;
  else
    {
      int bit;
      int count = 1;
      --tail_hits;
      for (bit = 1; bit & 0xff; bit <<= 1, ++kids)
	if (bit & hits)
	  count += count_buf_size (*kids, tail_hits);
      return count;
    }
}

void
assert_hits (struct summary* summary)
{
  struct summary **kids = summary->sum_kids;
  struct summary **end = &kids[8];

  assert (summary->sum_hits == NULL || *summary->sum_hits == 0);
  
  if (end[-1] == 0)
    while (*--end == 0)
      ;
  while (kids < end)
    assert_hits (*kids++);
}

void
write_hits (FILE *fp, struct summary *summary, unsigned char const *tail_hits)
{
  struct summary **kids;
  unsigned int hits = (summary->sum_hits ? *summary->sum_hits++ : *tail_hits);

  assert (hits);
  putc (hits, fp);

  kids = summary->sum_kids;
  if (*kids)
    {
      int bit;
      --tail_hits;
      for (bit = 1; (bit & 0xff) && *kids; bit <<= 1, ++kids)
	if (bit & hits)
	  write_hits (fp, *kids, tail_hits);
    }
}

void
sign_token (struct token *token)
{
  unsigned char *tok_hits = token->tok_hits;
  unsigned char *hits_sig = current_hits_signature;
  unsigned char *end = &current_hits_signature[MAX_LEVELS];
  struct summary *summary = summary_leaf;

  while (summary)
    {
      if (*tok_hits == 0)
	add_token_to_summary (summary, token);
      if (*tok_hits & *hits_sig)
	break;
      *tok_hits |= *hits_sig;
      summary = summary->sum_parent;
      tok_hits++;
      hits_sig++;
    }
  while (hits_sig < end)
    {
      if (*tok_hits & *hits_sig)
	break;
      *tok_hits |= *hits_sig;
      tok_hits++;
      hits_sig++;
    }
}

void
add_token_to_summary (struct summary *summary, struct token *token)
{
  unsigned long size = summary->sum_tokens_size;

  if (summary->sum_hits_count >= size)
    {
      size *= 2;
      summary->sum_tokens = REALLOC (summary->sum_tokens, struct token *, size);
      summary->sum_tokens_size = size;
    }
  summary->sum_tokens[summary->sum_hits_count++] = token;
}

int
bitsany (char const *s, int n)
{
  while (n--)
    if (*s++)
      return 1;

  return 0;
}

char *
bitsset (char *s1, char const *s2, int n)
{
  while (n--)
    *s1++ |= *s2++;

  return s1;
}

char *
bitsclr (char *s1, char const *s2, int n)
{
  while (n--)
    *s1++ &= ~*s2++;

  return s1;
}

#if 0

char *
bitsand (char *s1, char const *s2, int n)
{
  while (n--)
    *s1++ &= *s2++;

  return s1;
}

char *
bitsxor (char *s1, char const *s2, int n)
{
  while (n--)
    *s1++ ^= *s2++;

  return s1;
}

int
bitstst (char const *s1, char const *s2, int n)
{
  while (n--)
    if (*s1++ & *s2++)
      return 1;

  return 0;
}

#endif
