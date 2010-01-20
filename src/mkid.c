/* mkid.c -- build an identifer database
   Copyright (C) 1986, 1995-1996, 1999, 2007-2010 Free Software Foundation,
   Inc.
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
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

#include "alloca.h"
#include "argv-iter.h"
#include "closeout.h"
#include "dirname.h"
#include "error.h"
#include "inttostr.h"
#include "pathmax.h"
#include "progname.h"
#include "quote.h"
#include "quotearg.h"
#include "xalloc.h"

#include "xnls.h"
#include "idfile.h"
#include "idu-hash.h"
#include "scanners.h"
#include "iduglobal.h"

struct summary
{
  struct token **sum_tokens;
  unsigned char const *sum_hits;
  struct summary *sum_parent;
  union {
    struct summary *u_kids[8];	/* when sum_level > 0 */
#define sum_kids sum_u.u_kids
    struct member_file *u_files[8];	/* when sum_level == 0 */
  } sum_u;
  unsigned long sum_tokens_size;
  unsigned long sum_hits_count;
  int sum_free_index;
  int sum_level;
};

void usage (void);
static int ceil_log_8 (unsigned long n);
static int ceil_log_2 (unsigned long n);
static void assert_writeable (char const *file_name);
static void scan_files (struct idhead const *idhp);
static void scan_member_file (struct member_file const *member);
static void scan_member_file_1 (get_token_func_t get_token,
				void const *args, FILE *source_FILE);
static void report_statistics (void);
static void write_id_file (struct idhead *idhp);
static unsigned long token_hash_1 (void const *key);
static unsigned long token_hash_2 (void const *key);
static int token_hash_cmp (void const *x, void const *y);
static int token_qsort_cmp (void const *x, void const *y);
static void bump_current_hits_signature (void);
static void init_hits_signature (int i);
static void free_summary_tokens (void);
static void summarize (void);
static void init_summary (void);
static struct summary *make_sibling_summary (struct summary *summary);
static int count_vec_size (struct summary *summary,
			   unsigned char const *tail_hits);
static int count_buf_size (struct summary *summary,
			   unsigned char const *tail_hits);
static void assert_hits (struct summary* summary);
static void write_hits (FILE *fp, struct summary *summary,
			unsigned char const *tail_hits);
static void sign_token (struct token *token);
static void add_token_to_summary (struct summary *summary, struct token *token);

static struct hash_table token_table;

/* Miscellaneous statistics */
static unsigned long input_chars;
static unsigned long name_tokens;
static unsigned long number_tokens;
static unsigned long string_tokens;
static unsigned long literal_tokens;
static unsigned long comment_tokens;
static unsigned long occurrences;
static unsigned long hits_length = 0;
static unsigned long tokens_length = 0;
static unsigned long output_length = 0;

static int verbose_flag = 0;
static int statistics_flag = 0;

static int levels = 0;			/* ceil(log(8)) of file_name_count */

static unsigned char *current_hits_signature;
#define INIT_TOKENS_SIZE(level) (1 << ((level) + 13))
static struct summary *summary_root;
static struct summary *summary_leaf;

static char *lang_map_file_name = 0;
static int show_version = 0;
static int show_help = 0;
struct idhead idh;
static struct file_link *cw_dlink;

void usage (void) __attribute__((__noreturn__));
void
usage (void)
{
  fprintf (stderr, _("Try `%s --help' for more information.\n"),
	   program_name);
  exit (EXIT_FAILURE);
}

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  FILES0_FROM_OPTION = CHAR_MAX +1,
};

static struct option const long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "output", required_argument, 0, 'o' },
  { "include", required_argument, 0, 'i' },
  { "exclude", required_argument, 0, 'x' },
  { "lang-option", required_argument, 0, 'l' },
  { "lang-map", required_argument, 0, 'm' },
  { "default-lang", required_argument, 0, 'd' },
  { "prune", required_argument, 0, 'p' },
  { "verbose", no_argument, 0, 'v' },
  { "statistics", no_argument, 0, 's' },
  { "help", no_argument, &show_help, 1 },
  { "version", no_argument, &show_version, 1 },
  { "files0-from", required_argument, NULL, FILES0_FROM_OPTION },
  {NULL, 0, NULL, 0}
};

static void __attribute__((__noreturn__))
help_me (void)
{
  printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"), program_name);

  printf (_("\
Build an identifier database.\n\
  -o, --output=OUTFILE    file name of ID database output\n\
  -f, --file=OUTFILE      synonym for --output\n\
  -i, --include=LANGS     include languages in LANGS (default: \"C C++ asm\")\n\
  -x, --exclude=LANGS     exclude languages in LANGS\n\
  -l, --lang-option=L:OPT pass OPT as a default for language L (see below)\n\
  -m, --lang-map=MAPFILE  use MAPFILE to map file names onto source language\n\
  -d, --default-lang=LANG  make LANG the default source language\n\
  -p, --prune=NAMES       exclude the named files and/or directories\n\
  -v, --verbose           report per file statistics\n\
  -s, --statistics        report statistics at end of run\n\
\n\
      --files0-from=F     tokenize only the files specified by\n\
                           NUL-terminated names in file F\n\
\n\
       --help              display this help and exit\n\
      --version           output version information and exit\n\
\n\
FILE may be a file name, or a directory name to recursively search.\n\
If no FILE is given, the current directory is searched by default.\n\
Note that the `--include' and `--exclude' options are mutually-exclusive.\n\
\n\
The following arguments apply to the language-specific scanners:\n\
"));
  language_help_me ();
  printf (_("\nReport bugs to " PACKAGE_BUGREPORT "\n\n"));
  exit (EXIT_SUCCESS);
}

static void *heap_initial;
static void *heap_after_walk;
static void *heap_after_scan;

static void *get_process_heap(void)
{
#if HAVE_SBRK
  return sbrk(0);
#else
  return 0;
#endif /* HAVE_SBRK */
}

int
main (int argc, char **argv)
{
  bool ok;
  int nfiles;
  char *files_from = NULL;

  set_program_name (argv[0]);
  heap_initial = get_process_heap();
  idh.idh_file_name = DEFAULT_ID_FILE_NAME;

#if ENABLE_NLS
  /* Set locale according to user's wishes.  */
  setlocale (LC_ALL, "");

  /* Tell program which translations to use and where to find.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  atexit (close_stdout);

  for (;;)
    {
      int optc = getopt_long (argc, argv, "o:f:i:x:l:m:d:p:vVs",
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
	  include_languages (optarg);
	  break;

	case 'x':
	  exclude_languages (optarg);
	  break;

	case 'l':
	  language_save_arg (optarg);
	  break;

	case 'm':
	  lang_map_file_name = optarg;
	  break;

	case 'd':
	  set_default_language (optarg);
	  break;

	case 'p':
	  if (cw_dlink == 0)
	    cw_dlink = init_walker (&idh);
	  prune_file_names (optarg, cw_dlink);
	  break;

	case FILES0_FROM_OPTION:
	  files_from = optarg;
	  break;

	case 'V':
	  walker_verbose_flag = 1;
	case 'v':
	  verbose_flag = 1;
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
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    help_me ();

  nfiles = argc - optind;

  struct argv_iterator *ai;
  if (files_from)
    {
      /* When using --files0-from=F, you may not specify any files
	 on the command-line.  */
      if (nfiles != 0)
	{
	  error (0, 0, _("extra operand %s"), quote (argv[optind]));
	  fprintf (stderr, "%s\n",
		   _("file operands cannot be combined with --files0-from"));
	  usage();
	}

      if (! (strequ (files_from, "-") || freopen (files_from, "r", stdin)))
	error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
	       quote (files_from));

      ai = argv_iter_init_stream (stdin);
    }
  else
    {
      static char *cwd_only[2];
      cwd_only[0] = bad_cast (".");
      cwd_only[1] = NULL;
      char **files = (optind < argc ? argv + optind : cwd_only);
      ai = argv_iter_init_argv (files);
    }

  language_getopt ();
  assert_writeable (idh.idh_file_name);
  if (cw_dlink == 0)
    cw_dlink = init_walker (&idh);
  parse_language_map (lang_map_file_name);

  /* Walk the file and directory names given on the command line.  */
  ok = true;
  while (true)
    {
      bool skip_file = false;
      enum argv_iter_err ai_err;
      char *file_name = argv_iter (ai, &ai_err);
      if (ai_err == AI_ERR_EOF)
        break;
      if (!file_name)
        {
          switch (ai_err)
            {
            case AI_ERR_READ:
              error (0, errno, _("%s: read error"), quote (files_from));
              continue;

            case AI_ERR_MEM:
              xalloc_die ();

            default:
              assert (!"unexpected error code from argv_iter");
            }
        }
      if (files_from && STREQ (files_from, "-") && STREQ (file_name, "-"))
        {
          /* Give a better diagnostic in an unusual case:
             printf - | du --files0-from=- */
          error (0, 0, _("when reading file names from stdin, "
                         "no file name of %s allowed"),
                 quote (file_name));
          skip_file = true;
        }

      /* Report and skip any empty file names.  */
      if (!file_name[0])
        {
          /* Diagnose a zero-length file name.  When it's one
             among many, knowing the record number may help.
             FIXME: currently print the record number only with
             --files0-from=FILE.  Maybe do it for argv, too?  */
          if (files_from == NULL)
            error (0, 0, "%s", _("invalid zero-length file name"));
          else
            {
              /* Using the standard `filename:line-number:' prefix here is
                 not totally appropriate, since NUL is the separator, not NL,
                 but it might be better than nothing.  */
              unsigned long int file_number = argv_iter_n_args (ai);
              error (0, 0, "%s:%lu: %s", quotearg_colon (files_from),
                     file_number, _("invalid zero-length file name"));
            }
          skip_file = true;
        }

      if (skip_file)
        ok = false;
      else
        {
          struct file_link *flink = parse_file_name (file_name, cw_dlink);
          if (flink)
            walk_flink (flink, 0);
	  /* FIXME: walk_flink can fail, so should return status.
	     Then caller can continue with other arguments.  */
        }
    }
  argv_iter_free (ai);

  heap_after_walk = get_process_heap();

  mark_member_file_links (&idh);
  log_8_member_files = ceil_log_8 (idh.idh_member_file_table.ht_fill);

  /* hack start: when scanning a single file, log_8_member_files results 0.
     Adjust to 1 in this case, or xmalloc will be called for 0 bytes,
     and later the struct token will have no 'hits' field.
     This would cause a crash
     (see testsuite/consistency, testsuite/single_file_token_bug.c)

     <claudio@gnu.org 2005>
  */

  if (log_8_member_files == 0)
    log_8_member_files = 1;

  /* hack end */

  current_hits_signature = xmalloc (log_8_member_files);

  /* If scannable files were given, then scan them.  */
  if (idh.idh_member_file_table.ht_fill)
    {
      scan_files (&idh);
      heap_after_scan = get_process_heap();

      free_summary_tokens ();
      free (token_table.ht_vec);
      chdir_to_link (cw_dlink);
      write_id_file (&idh);

      if (statistics_flag)
	report_statistics ();
    }
  else
    error (0, 0, _("nothing to do"));

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Return the integer ceiling of the base-8 logarithm of N.  */

static int
ceil_log_8 (unsigned long n)
{
  int log_8 = 0;

  n--;
  while (n)
    {
      log_8++;
      n >>= 3;
    }
  return log_8;
}

/* Return the integer ceiling of the base-2 logarithm of N.  */

static int
ceil_log_2 (unsigned long n)
{
  int log_2 = 0;

  n--;
  while (n)
    {
      log_2++;
      n >>= 1;
    }
  return log_2;
}

/* Ensure that FILE_NAME can be written.  If it exists, make sure it's
   writable.  If it doesn't exist, make sure it can be created.  Exit
   with a diagnostic if this is not so.  */

static void
assert_writeable (char const *filename)
{
  if (access (filename, 06) < 0)
    {
      if (errno == ENOENT)
	{
	  char *dirname = dir_name (filename);
	  if (access (dirname, 06) < 0)
	    error (EXIT_FAILURE, errno, _("can't create `%s' in `%s'"),
		   base_name (filename), dirname);
	  free(dirname);
	}
      else
	error (EXIT_FAILURE, errno, _("can't modify `%s'"), filename);
    }
}

/* Iterate over all eligible files (the members of the set of scannable files).
   Create a tree8 to store the set of files where a token occurs.  */

static void
scan_files (struct idhead const *idhp)
{
  struct member_file **members_0
    = (struct member_file **) hash_dump (&idhp->idh_member_file_table,
					 0, member_file_qsort_compare);
  struct member_file **end = &members_0[idhp->idh_member_file_table.ht_fill];
  struct member_file **members = members_0;
  int n = idhp->idh_member_file_table.ht_fill;

  n = n * ceil_log_2 (n) * 16;
  if (n == 0)
    n = 1024;
  else if (n > 1024*1024)
    n = 1024*1024;

  hash_init (&token_table, n, token_hash_1, token_hash_2, token_hash_cmp);
  if (verbose_flag) {
    char offstr[INT_BUFSIZE_BOUND(off_t)];

    printf ("files=%ld, largest=%s, slots=%lu\n",
	    idhp->idh_member_file_table.ht_fill,
	    offtostr(largest_member_file, offstr),
	    token_table.ht_size);
  }
  init_hits_signature (0);
  init_summary ();
  obstack_init (&tokens_obstack);

  if (largest_member_file > MAX_LARGEST_MEMBER_FILE)
    largest_member_file = MAX_LARGEST_MEMBER_FILE;
  scanner_buffer = xmalloc (largest_member_file + 1);

  for (;;)
    {
      const struct member_file *member = *members++;
      scan_member_file (member);
      if (members == end)
	break;
      if (current_hits_signature[0] & 0x80)
	summarize ();
      bump_current_hits_signature ();
    }

  free (scanner_buffer);
  free (members_0);
}

/* Open a file and scan it.  */

static void
scan_member_file (struct member_file const *member)
{
  struct lang_args const *lang_args = member->mf_lang_args;
  struct language const *lang = lang_args->la_language;
  get_token_func_t get_token = lang->lg_get_token;
  struct file_link *flink = member->mf_link;
  struct stat st;
  FILE *source_FILE;

  chdir_to_link (flink->fl_parent);
  source_FILE = fopen (flink->fl_name, "r");
  if (source_FILE)
    {
      char *file_name = alloca (PATH_MAX);
      if (statistics_flag)
	{
	  if (fstat (fileno (source_FILE), &st) < 0)
	    {
	      maybe_relative_file_name (file_name, flink, cw_dlink);
	      error (0, errno, _("can't stat `%s'"), file_name);
	    }
	  else
	    input_chars += st.st_size;
	}
      if (verbose_flag)
	{
	  maybe_relative_file_name (file_name, flink, cw_dlink);
	  printf ("%ld: %s: %s", member->mf_index, lang->lg_name, file_name);
	  fflush (stdout);
	}
      scan_member_file_1 (get_token, lang_args->la_args_digested, source_FILE);
      if (verbose_flag)
	putchar ('\n');
      fclose (source_FILE);
    }
  else
    error (0, errno, _("can't open `%s'"), flink->fl_name);
}

/* Iterate over all tokens in the file, and merge the file's tree8
   signature into the token table entry.  */

static void
scan_member_file_1 (get_token_func_t get_token, void const *args, FILE *source_FILE)
{
  struct token **slot;
  struct token *token;
  int flags;
  int new_tokens = 0;
  int distinct_tokens = 0;

  while ((token = (*get_token) (source_FILE, args, &flags)) != NULL)
    {
      if (*TOKEN_NAME (token) == '\0') {
	obstack_free (&tokens_obstack, token);
	continue;
      }
      slot = (struct token **) hash_find_slot (&token_table, token);
      if (HASH_VACANT (*slot))
	{
	  token->tok_flags = flags;
	  token->tok_count = 1;
	  memset (TOKEN_HITS (token), 0, log_8_member_files);
	  sign_token (token);
	  if (verbose_flag)
	    {
	      distinct_tokens++;
	      new_tokens++;
	    }
	  hash_insert_at (&token_table, token, slot);
	}
      else
	{
	  obstack_free (&tokens_obstack, token);
	  token = *slot;
	  token->tok_flags |= flags;
	  if (token->tok_count < USHRT_MAX)
	    token->tok_count++;
	  if (!(TOKEN_HITS (token)[0] & current_hits_signature[0]))
	    {
	      sign_token (token);
	      if (verbose_flag)
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

static void
report_statistics (void)
{
  printf (_("Name=%ld, "), name_tokens);
  printf (_("Number=%ld, "), number_tokens);
  printf (_("String=%ld, "), string_tokens);
  printf (_("Literal=%ld, "), literal_tokens);
  printf (_("Comment=%ld\n"), comment_tokens);

  printf (_("Files=%ld, "), idh.idh_files);
  printf (_("Tokens=%ld, "), occurrences);
  printf (_("Bytes=%ld Kb, "), input_chars / 1024);
  printf (_("Heap=%llu+%llu Kb, "),
	  (unsigned long long) ((char *) heap_after_scan
				- (char *) heap_after_walk) / 1024,
	  (unsigned long long) ((char *) heap_after_walk
				- (char *) heap_initial) / 1024);
  printf (_("Output=%ld (%ld tok, %ld hit)\n"),
	  output_length, tokens_length, hits_length);

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

static void
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
  tokens = xnrealloc (summary_root->sum_tokens,
		      token_table.ht_fill, sizeof *tokens);
  qsort (tokens, token_table.ht_fill, sizeof (struct token *), token_qsort_cmp);

  if (verbose_flag)
    printf (_("Writing `%s'...\n"), idhp->idh_file_name);
  idhp->idh_FILE = fopen (idhp->idh_file_name, "w+b");
  if (idhp->idh_FILE == NULL)
    error (EXIT_FAILURE, errno, _("can't create `%s'"), idhp->idh_file_name);

  idhp->idh_magic[0] = IDH_MAGIC_0;
  idhp->idh_magic[1] = IDH_MAGIC_1;
  idhp->idh_version = IDH_VERSION;
  idhp->idh_flags = IDH_COUNTS;

  /* write out the list of pathnames */

  fseek (idhp->idh_FILE, sizeof_idhead (), 0);
  off_t off = ftello (idhp->idh_FILE);
  if (UINT32_MAX < off)
    error (EXIT_FAILURE, 0, _("internal limitation: offset of 2^32 or larger"));
  idhp->idh_flinks_offset = off;
  serialize_file_links (idhp);

  /* write out the list of identifiers */

  putc ('\0', idhp->idh_FILE);
  putc ('\0', idhp->idh_FILE);
  off = ftello (idhp->idh_FILE);
  if (UINT32_MAX < off)
    error (EXIT_FAILURE, 0, _("internal limitation: offset of 2^32 or larger"));
  idhp->idh_tokens_offset = off;

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

      fputs (TOKEN_NAME (token), idhp->idh_FILE);
      putc ('\0', idhp->idh_FILE);
      if (token->tok_count > 0xff)
	token->tok_flags |= TOK_SHORT_COUNT;
      putc (token->tok_flags, idhp->idh_FILE);
      putc (token->tok_count & 0xff, idhp->idh_FILE);
      if (token->tok_flags & TOK_SHORT_COUNT)
	putc (token->tok_count >> 8, idhp->idh_FILE);

      vec_size = count_vec_size (summary_root, TOKEN_HITS (token) + levels);
      buf_size = count_buf_size (summary_root, TOKEN_HITS (token) + levels);
      hits_length += buf_size;
      tok_size = strlen (TOKEN_NAME (token)) + 1;
      tokens_length += tok_size;
      buf_size += tok_size + sizeof (token->tok_flags) + sizeof (token->tok_count) + 2;
      if (buf_size > max_buf_size)
	max_buf_size = buf_size;
      if (vec_size > max_vec_size)
	max_vec_size = vec_size;

      write_hits (idhp->idh_FILE, summary_root, TOKEN_HITS (token) + levels);
      putc ('\0', idhp->idh_FILE);
      putc ('\0', idhp->idh_FILE);
    }
  assert_hits (summary_root);
  idhp->idh_tokens = token_table.ht_fill;
  off = ftello (idhp->idh_FILE);
  if (UINT32_MAX < off)
    error (EXIT_FAILURE, 0, _("internal limitation: offset of 2^32 or larger"));
  output_length = off;
  idhp->idh_end_offset = output_length - 2;
  idhp->idh_buf_size = max_buf_size;
  idhp->idh_vec_size = max_vec_size;

  write_idhead (&idh);
  if (ferror (idhp->idh_FILE) || fclose (idhp->idh_FILE) != 0)
    error (EXIT_FAILURE, errno, _("error closing `%s'"), idhp->idh_file_name);
}

/* Define primary and secondary hash and comparison functions for the
   token table.  */

static unsigned long
token_hash_1 (void const *key)
{
  return_STRING_HASH_1 (TOKEN_NAME ((struct token const *) key));
}

static unsigned long
token_hash_2 (void const *key)
{
  return_STRING_HASH_2 (TOKEN_NAME ((struct token const *) key));
}

static int
token_hash_cmp (void const *x, void const *y)
{
  return_STRING_COMPARE (TOKEN_NAME ((struct token const *) x),
			 TOKEN_NAME ((struct token const *) y));
}

static int
token_qsort_cmp (void const *x, void const *y)
{
  return_STRING_COMPARE (TOKEN_NAME (*(struct token const *const *) x),
			 TOKEN_NAME (*(struct token const *const *) y));
}


/****************************************************************************/

/* Advance the tree8 hit signature that corresponds to the file
   we are now scanning.  */

static void
bump_current_hits_signature (void)
{
  unsigned char *hits = current_hits_signature;
  while (*hits & 0x80)
    *hits++ = 1;
  *hits <<= 1;
}

/* Initialize the tree8 hit signature for the first file we scan.  */

static void
init_hits_signature (int i)
{
  unsigned char *hits = current_hits_signature;
  unsigned char const *end = current_hits_signature + log_8_member_files;
  while (hits < end)
    {
      *hits = 1 << (i & 7);
      i >>= 3;
      hits++;
    }
}

static void
free_summary_tokens (void)
{
  struct summary *summary = summary_leaf;
  while (summary != summary_root)
    {
      free (summary->sum_tokens);
      summary = summary->sum_parent;
    }
}

static void
summarize (void)
{
  unsigned char const *hits_sig = current_hits_signature;
  struct summary *summary = summary_leaf;

  do
    {
      unsigned long count = summary->sum_hits_count;
      unsigned char *hits = xmalloc (count + 1);
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
	  unsigned char *hit = TOKEN_HITS (*tokens++) + level;
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

static void
init_summary (void)
{
  unsigned long size = INIT_TOKENS_SIZE (0);
  summary_root = summary_leaf = xcalloc (1, sizeof(struct summary));
  summary_root->sum_tokens_size = size;
  summary_root->sum_tokens = xmalloc (sizeof(struct token *) * size);
}

static struct summary *
make_sibling_summary (struct summary *summary)
{
  struct summary *parent = summary->sum_parent;
  size_t size;

  if (parent == NULL)
    {
      levels++;
      summary_root = summary->sum_parent = parent
	= xcalloc (1, sizeof(struct summary));
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
	  parent->sum_tokens = xnrealloc (summary->sum_tokens, size,
					  sizeof *summary->sum_tokens);
	}
      summary->sum_tokens = 0;
    }
  if (parent->sum_free_index == 8)
    parent = make_sibling_summary (parent);
  summary = xcalloc (1, sizeof(struct summary));
  summary->sum_level = parent->sum_level - 1;
  parent->sum_kids[parent->sum_free_index++] = summary;
  summary->sum_parent = parent;
  size = INIT_TOKENS_SIZE (summary->sum_level);
  summary->sum_tokens_size = size;
  summary->sum_tokens = xmalloc (sizeof(struct token *) * size);
  return summary;
}

static int
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

static int
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

static void
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

static void
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

static void
sign_token (struct token *token)
{
  unsigned char *tok_hits = TOKEN_HITS (token);
  unsigned char *hits_sig = current_hits_signature;
  unsigned char *end = current_hits_signature + log_8_member_files;
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

static void
add_token_to_summary (struct summary *summary, struct token *token)
{
  size_t size = summary->sum_tokens_size;

  if (summary->sum_hits_count >= size)
    {
      summary->sum_tokens = x2nrealloc (summary->sum_tokens, &size,
					sizeof *summary->sum_tokens);
      summary->sum_tokens_size = size;
    }
  summary->sum_tokens[summary->sum_hits_count++] = token;
}
