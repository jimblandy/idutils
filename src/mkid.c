/* mkid.c -- build an identifer database
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
#include <getopt.h>

#include <config.h>
#include "system.h"
#include "pathmax.h"
#include "strxtra.h"
#include "alloc.h"
#include "idfile.h"
#include "token.h"
#include "bitops.h"
#include "misc.h"
#include "filenames.h"
#include "hash.h"
#include "scanners.h"
#include "error.h"

struct summary
{
  struct token **sum_tokens;
  unsigned char const *sum_hits;
  struct summary *sum_parent;
  union {
    struct summary *u_kids[8];	/* when sum_level > 0 */
#define sum_kids sum_u.u_kids
    struct member_file *u_files[8];	/* when sum_level == 0 */
#define sum_files sum_u.u_files
  } sum_u;
  unsigned long sum_tokens_size;
  unsigned long sum_hits_count;
  int sum_free_index;
  int sum_level;
};

void usage __P((void));
static void help_me __P((void));
int main __P((int argc, char **argv));
void assert_writeable __P((char const *file_name));
void scan_files __P((struct idhead *idhp));
void scan_member_file __P((struct member_file const *member));
void scan_member_file_1 __P((get_token_func_t get_token, void const *args, FILE *source_FILE));
void report_statistics __P((void));
void write_id_file __P((struct idhead *idhp));
unsigned long token_hash_1 __P((void const *key));
unsigned long token_hash_2 __P((void const *key));
int token_hash_cmp __P((void const *x, void const *y));
int token_qsort_cmp __P((void const *x, void const *y));
void bump_current_hits_signature __P((void));
void init_hits_signature __P((int i));
void free_summary_tokens __P((void));
void summarize __P((void));
void init_summary __P((void));
struct summary *make_sibling_summary __P((struct summary *summary));
int count_vec_size __P((struct summary *summary, unsigned char const *tail_hits));
int count_buf_size __P((struct summary *summary, unsigned char const *tail_hits));
void assert_hits __P((struct summary* summary));
void write_hits __P((FILE *fp, struct summary *summary, unsigned char const *tail_hits));
void sign_token __P((struct token *token));
void add_token_to_summary __P((struct summary *summary, struct token *token));

struct hash_table token_table;

/* Miscellaneous statistics */
size_t input_chars;
size_t name_tokens;
size_t number_tokens;
size_t string_tokens;
size_t literal_tokens;
size_t comment_tokens;
size_t occurrences;
size_t hits_length = 0;
size_t tokens_length = 0;
size_t output_length = 0;

int verbose_flag = 0;
int statistics_flag = 0;

int file_name_count = 0;	/* # of files in database */
int levels = 0;			/* ceil(log(8)) of file_name_count */

unsigned char current_hits_signature[MAX_LEVELS];
#define INIT_TOKENS_SIZE(level) (1 << ((level) + 13))
struct summary *summary_root;
struct summary *summary_leaf;

char const *program_name;

char *include_languages = 0;
char *exclude_languages = 0;
char *lang_map_file_name = 0;
int show_version = 0;
int show_help = 0;
struct idhead idh;

void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (1);
}

static struct option const long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "output", required_argument, 0, 'o' },
  { "include", required_argument, 0, 'i' },
  { "exclude", required_argument, 0, 'r' },
  { "lang-arg", required_argument, 0, 'l' },
  { "lang-map", required_argument, 0, 'm' },
  { "verbose", no_argument, 0, 'v' },
  { "statistics", no_argument, 0, 's' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { 0 }
};

static void
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... [FILE]...\n"),
	  program_name);

  printf (_("\
Build an identifier database.\n\
  -o, --output=OUTFILE    file name of ID database output\n\
  -f, --file=OUTFILE      synonym for --output\n\
  -i, --include=LANGS     include languages in LANGS (default: \"C C++ asm\")\n\
  -x, --exclude=LANGS     exclude languages in LANGS\n\
  -l, --lang-arg=LANG:ARG pass ARG as a default for LANG (see below)\n\
  -m, --lang-map=MAPFILE  use MAPFILE to map file names onto source language\n\
  -v, --verbose           report progress and as files are scanned\n\
  -s, --statistics        report statistics at end of run\n\
\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
\n\
FILE may be a file name, or a directory name to recursively search.\n\
The `--include' and `--exclude' options are mutually-exclusive.\n\
\n\
The following arguments apply to the language-specific scanners:\n\
"));
  language_help_me ();
  exit (0);
}

#if !HAVE_DECL_SBRK
extern void *sbrk ();
#endif
char const *heap_initial;
char const *heap_after_walk;
char const *heap_after_scan;

int
main (int argc, char **argv)
{
  program_name = argv[0];
  idh.idh_file_name = ID_FILE_NAME;
  for (;;)
    {
      int optc = getopt_long (argc, argv, "o:f:i:x:l:m:uvs",
			      long_options, (int *) 0);
      if (optc < 0)
	break;
      switch (optc)
	{
	case 0:
	  break;

	case 'o':
	case 'f':
	  idh.idh_file_name = optarg;
	  break;

	case 'i':
	  include_languages = optarg;
	  break;

	case 'x':
	  exclude_languages = optarg;
	  break;

	case 'l':
	  language_save_arg (optarg);
	  break;
	  
	case 'm':
	  lang_map_file_name = optarg;
	  break;

	case 'v':
	  verbose_flag = 1;
	  break;

	case 's':
	  statistics_flag = 1;
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
  
  argc -= optind;
  argv += optind;
  language_getopt ();

  assert_writeable (idh.idh_file_name);

  if (argc == 0)
    {
      argc++;
      *(char const **)--argv = ".";
    }
  heap_initial = (char const *) sbrk (0);
  init_idh_obstacks (&idh);
  init_idh_tables (&idh);
  parse_language_map (lang_map_file_name);
  
  {
    struct file_link *cwd_link = get_current_dir_link ();
    while (argc--)
      walk_flink (parse_file_name (*argv++, cwd_link), 0);
    mark_member_file_links (&idh);
    heap_after_walk = (char const *) sbrk (0);
    scan_files (&idh);
    heap_after_scan = sbrk (0);
    free_summary_tokens ();
    free (token_table.ht_vec);
    chdir_to_link (cwd_link);
    write_id_file (&idh);
  }
  if (statistics_flag)
    report_statistics ();
  exit (0);
}

void
assert_writeable (char const *file_name)
{
  if (access (file_name, 06) < 0)
    {
      if (errno == ENOENT)
	{
	  char const *dir_name = dirname (file_name);
	  if (access (dir_name, 06) < 0)
	    error (1, errno, _("can't create `%s' in `%s'"),
		   basename (file_name), dir_name);
	}
      else
	error (1, errno, _("can't modify `%s'"), file_name);
    }
}

void
scan_files (struct idhead *idhp)
{
  struct member_file **members_0
    = (struct member_file **) hash_dump (&idhp->idh_member_file_table,
					 0, member_file_qsort_compare);
  struct member_file **end = &members_0[idhp->idh_member_file_table.ht_fill];
  struct member_file **members;

  hash_init (&token_table, idhp->idh_member_file_table.ht_fill * 64,
	     token_hash_1, token_hash_2, token_hash_cmp);
  init_hits_signature (0);
  init_summary ();
  obstack_init (&tokens_obstack);

  for (members = members_0; members < end; members++)
    scan_member_file (*members);
  free (members_0);
}

void
scan_member_file (struct member_file const *member)
{
  struct lang_args const *lang_args = member->mf_lang_args;
  struct language const *lang = lang_args->la_language;
  get_token_func_t get_token = lang->lg_get_token;
  struct file_link *flink = member->mf_link;
  struct stat st;
  FILE *source_FILE;
  size_t bytes;

  chdir_to_link (flink->fl_parent);
  source_FILE = open_source_FILE (flink->fl_name);
  if (source_FILE)
    {
      char buf[PATH_MAX];
      if (verbose_flag)
	{
	  printf ("%d: %s: %s", member->mf_index, lang->lg_name,
		  absolute_path (buf, flink));
	  fflush (stdout);
	}
      if (fstat (fileno (source_FILE), &st) < 0)
	error (0, errno, _("can't stat `%s'"), absolute_path (buf, flink));
      else
	{
	  bytes = st.st_size;
	  input_chars += bytes;
	}
      scan_member_file_1 (get_token, lang_args->la_args_digested, source_FILE);
      if (verbose_flag)
	putchar ('\n');
      close_source_FILE (source_FILE);
    }
  if (current_hits_signature[0] & 0x80)
    summarize ();
#if 0
  if (member->mf_index < file_name_count)
#endif
    bump_current_hits_signature ();
}

void
scan_member_file_1 (get_token_func_t get_token, void const *args, FILE *source_FILE)
{
  struct stat st;
  struct token **slot;
  struct token *token;
  int flags;
  int bytes = 0;
  int total_tokens = 0;
  int new_tokens = 0;
  int distinct_tokens = 0;

  while ((token = (*get_token) (source_FILE, args, &flags)) != NULL)
    {
      if (*token->tok_name == '\0') {
	obstack_free (&tokens_obstack, token);
	continue;
      }
      total_tokens++;
      slot = (struct token **) hash_find_slot (&token_table, token);
      if (HASH_VACANT (*slot))
	{
	  token->tok_count = 1;
	  memset (token->tok_hits, 0, sizeof (token->tok_hits));
	  token->tok_flags = flags;
	  sign_token (token);
	  distinct_tokens++;
	  new_tokens++;
	  hash_insert_at (&token_table, token, slot);
	}
      else
	{
	  obstack_free (&tokens_obstack, token);
	  token = *slot;
	  token->tok_flags |= flags;
	  if (token->tok_count < USHRT_MAX)
	    token->tok_count++;
	  if (!(token->tok_hits[0] & current_hits_signature[0]))
	    {
	      sign_token (token);
	      distinct_tokens++;
	    }
	}
    }
  if (verbose_flag)
    {
      printf (_("  new = %d/%d"), new_tokens, distinct_tokens);
      if (distinct_tokens != 0)
	printf (" = %.0f%%", 100.0 * (double) new_tokens / (double) distinct_tokens);
    }
}

void
report_statistics (void)
{
  printf (_("Name=%ld, "), name_tokens);
  printf (_("Number=%ld, "), number_tokens);
  printf (_("String=%ld, "), string_tokens);
  printf (_("Literal=%ld, "), literal_tokens);
  printf (_("Comment=%ld\n"), comment_tokens);

  printf (_("Files=%d, "), idh.idh_files);
  printf (_("Tokens=%ld, "), occurrences);
  printf (_("Bytes=%ld Kb, "), input_chars / 1024);
  printf (_("Heap=%ld+%ld Kb, "), (heap_after_scan - heap_after_walk) / 1024,
	  (heap_after_walk - heap_initial) / 1024);
  printf (_("Output=%ld (%ld tok, %ld hit)\n"), output_length, tokens_length, hits_length);

  hash_print_stats (&token_table, stdout);
  printf (_(", Freq=%ld/%ld=%.2f\n"), occurrences, token_table.ht_fill,
	  (double) occurrences / (double) token_table.ht_fill);
}

/* As the database is written, may need to adjust the file names.  If
   we are generating the ID file in a remote directory, then adjust
   the file names to be relative to the location of the ID database.

   (This would be a common useage if you want to make a database for a
   directory which you have no write access to, so you cannot create
   the ID file.)  */
void
write_id_file (struct idhead *idhp)
{
  struct token **tokens;
  int i;
  int buf_size;
  int vec_size;
  int tok_size;
  int max_buf_size = 0;
  int max_vec_size = 0;

  if (verbose_flag)
    printf (_("Sorting tokens...\n"));
  assert (summary_root->sum_hits_count == token_table.ht_fill);
  tokens = REALLOC (summary_root->sum_tokens, struct token *, token_table.ht_fill);
  qsort (tokens, token_table.ht_fill, sizeof (struct token *), token_qsort_cmp);

  if (verbose_flag)
    printf (_("Writing `%s'...\n"), idhp->idh_file_name);
  idhp->idh_FILE = fopen (idhp->idh_file_name, "w+b");
  if (idhp->idh_FILE == NULL)
    error (1, errno, _("can't create `%s'"), idhp->idh_file_name);

  idhp->idh_magic[0] = IDH_MAGIC_0;
  idhp->idh_magic[1] = IDH_MAGIC_1;
  idhp->idh_version = IDH_VERSION;
  idhp->idh_flags = IDH_COUNTS;

  /* write out the list of pathnames */

  fseek (idhp->idh_FILE, sizeof_idhead (), 0);
  idhp->idh_flinks_offset = ftell (idhp->idh_FILE);
  serialize_file_links (idhp);

  /* write out the list of identifiers */

  putc ('\0', idhp->idh_FILE);
  putc ('\0', idhp->idh_FILE);
  idhp->idh_tokens_offset = ftell (idhp->idh_FILE);

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

      fputs (token->tok_name, idhp->idh_FILE);
      putc ('\0', idhp->idh_FILE);
      if (token->tok_count > 0xff)
	token->tok_flags |= TOK_SHORT_COUNT;
      putc (token->tok_flags, idhp->idh_FILE);
      putc (token->tok_count & 0xff, idhp->idh_FILE);
      if (token->tok_flags & TOK_SHORT_COUNT)
	putc (token->tok_count >> 8, idhp->idh_FILE);

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

      write_hits (idhp->idh_FILE, summary_root, token->tok_hits + levels);
      putc ('\0', idhp->idh_FILE);
      putc ('\0', idhp->idh_FILE);
    }
  assert_hits (summary_root);
  idhp->idh_tokens = token_table.ht_fill;
  output_length = ftell (idhp->idh_FILE);
  idhp->idh_end_offset = output_length - 2;
  idhp->idh_buf_size = max_buf_size;
  idhp->idh_vec_size = max_vec_size;

  write_idhead (&idh);
  fclose (idhp->idh_FILE);
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


/****************************************************************************/

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
	printf (_("level %d: %ld/%ld = %.0f%%\n"),
		summary->sum_level, count, init_size,
		100.0 * (double) count / (double) init_size);

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
