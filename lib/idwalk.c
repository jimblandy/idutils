#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fnmatch.h>
#ifndef FNM_FILE_NAME
#define FNM_FILE_NAME FNM_PATHNAME
#endif

#define DEBUG(args) /* printf args */

#include <config.h>
#include "system.h"
#include "idfile.h"
#include "error.h"
#include "alloc.h"
#include "dynvec.h"
#include "strxtra.h"
#include "scanners.h"
#include "pathmax.h"

int walk_dir __P((struct file_link *dir_link));
void walk_flink __P((struct file_link *flink, struct dynvec *sub_dirs_vec));
struct member_file *get_member_file __P((struct file_link *flink));
struct lang_args *get_lang_args __P((struct file_link const *flink));
int walk_sub_dirs __P((struct dynvec *sub_dirs_vec));
int classify_link __P((struct file_link *flink, struct stat *st));
struct file_link *get_link_from_dirent __P((struct dirent *dirent, struct file_link *parent));
struct file_link *make_link_from_dirent __P((struct dirent *dirent, struct file_link *parent));
struct file_link *get_link_from_string __P((char const *name, struct file_link *parent));
struct file_link *make_link_from_string __P((char const *name, struct file_link *parent));
static int same_as_dot __P((char const *cwd));
int chdir_to_link __P((struct file_link* dir_link));
struct file_link const **fill_link_vector __P((struct file_link const **vec_buf, struct file_link const *flink));
struct file_link const **fill_link_vector_1 __P((struct file_link const **vec_buf, struct file_link const *flink));
char const *maybe_relative_path __P((char *buffer, struct file_link const *to_link, struct file_link const *from_link));
char *fill_dot_dots __P((char *buf, int levels));
char *absolute_path __P((char *buffer, struct file_link const *flink));
static char *absolute_path_1 __P((char *buffer, struct file_link const *flink));
unsigned long member_file_hash_1 __P((void const *key));
unsigned long member_file_hash_2 __P((void const *key));
int member_file_hash_compare __P((void const *x, void const *y));
unsigned long file_link_hash_1 __P((void const *key));
unsigned long file_link_hash_2 __P((void const *key));
int file_link_hash_compare __P((void const *x, void const *y));
int file_link_qsort_compare __P((void const *x, void const *y));
int links_depth __P((struct file_link const *flink));
unsigned long dev_ino_hash_1 __P((void const *key));
unsigned long dev_ino_hash_2 __P((void const *key));
int dev_ino_hash_compare __P((void const *x, void const *y));
int symlink_ancestry __P((struct file_link *flink));

/* Interpret `HAVE_LINK' as meaning `UN*X style' directory structure
   (e.g., A single root called `/', with `/' separating links), and
   !HAVE_LINK as `DOS|OS/2|Windows style' (e.g., Multiple root volues
   named `x:', with `\' separating links).  */

#if HAVE_LINK
struct file_link *find_alias_link __P((struct file_link *flink, struct stat *st));
struct member_file *maybe_get_member_file __P((struct file_link *flink, struct stat *st));
struct member_file *find_member_file __P((struct file_link const *flink));
# define IS_ABSOLUTE(_dir_) ((_dir_)[0] == '/')
# define SLASH_STRING "/"
# define SLASH_CHAR '/'
# define DOT_DOT_SLASH "../"
# define MAYBE_FNM_CASEFOLD 0
#else
/* NEEDSWORK: prefer forward-slashes as a user-configurable option.  */
# define IS_ABSOLUTE(_dir_) ((_dir_)[1] == ':')
# define SLASH_STRING "\\/"
# define SLASH_CHAR '\\'
# define DOT_DOT_SLASH "..\\"
# define MAYBE_FNM_CASEFOLD FNM_CASEFOLD
#endif

#define IS_DOT(s) ((s)[0] == '.' && (s)[1] == '\0')
#define IS_DOT_DOT(s) ((s)[0] == '.' && (s)[1] == '.' && (s)[2] == '\0')
#define IS_DOT_or_DOT_DOT(s) \
    (((s)[0] == '.') && (((s)[1] == '\0') || ((s)[1] == '.' && (s)[2] == '\0')))

static struct file_link *current_dir_link = 0;

char* xgetcwd __P((void));

/****************************************************************************/
/* Walk the file-system tree rooted at `dir_link', looking for files
   that are eligible for scanning.  */

int
walk_dir (struct file_link *dir_link)
{
  char buf[PATH_MAX];
  int scannable_files;
  struct dynvec *sub_dirs_vec;
  DIR *dirp;

  if (!chdir_to_link (dir_link))
    return 0;
  dirp = opendir (".");
  if (dirp == 0)
    {
      error (0, errno, _("can't read directory `%s' (`.' from `%s')"),
	     absolute_path (buf, dir_link), xgetcwd ());
      return 0;
    }
  sub_dirs_vec = make_dynvec (32);
  scannable_files = 0;
  for (;;)
    {
      struct file_link *flink;
      struct dirent *dirent = readdir (dirp);

      if (dirent == 0)
	break;
      if (IS_DOT_or_DOT_DOT (dirent->d_name))
	continue;

      flink = get_link_from_dirent (dirent, dir_link);
      walk_flink (flink, sub_dirs_vec);
    }
  closedir (dirp);

  scannable_files += walk_sub_dirs (sub_dirs_vec);
  dynvec_free (sub_dirs_vec);
  return scannable_files;
}

/* Walk the directories found by walk_dir, calling walk_dir
   recursively for each directory. */

int
walk_sub_dirs (struct dynvec *sub_dirs_vec)
{
  struct file_link **sub_dirs;
  struct file_link **sub_dirs_end;
  int total_scannable_files = 0;

  dynvec_freeze (sub_dirs_vec);
  sub_dirs_end = (struct file_link **)
    &sub_dirs_vec->dv_vec[sub_dirs_vec->dv_fill];
  sub_dirs = (struct file_link **) sub_dirs_vec->dv_vec;
  for ( ; sub_dirs < sub_dirs_end; sub_dirs++)
    {
      struct file_link *sub_dir_link = *sub_dirs;
      int scannable_files = walk_dir (sub_dir_link);
      if (scannable_files)
	total_scannable_files += scannable_files;
    }
  return total_scannable_files;
}

void
walk_flink (struct file_link *flink, struct dynvec *sub_dirs_vec)
{
  char buf[PATH_MAX];
  struct stat st;
  unsigned int old_flags;
  unsigned int new_flags;

  new_flags = classify_link (flink, &st);
  if (new_flags == 0)
    return;

  old_flags = flink->fl_flags;
  if ((old_flags & FL_TYPE_MASK)
      && (old_flags & FL_TYPE_MASK) != (new_flags & FL_TYPE_MASK))
    error (0, 0, _("notice: `%s' was a %s, but is now a %s!"),
	   absolute_path (buf, flink),
	   (FL_IS_FILE (old_flags) ? _("file") : _("directory")),
	   (FL_IS_FILE (new_flags) ? _("file") : _("directory")));

  flink->fl_flags = (old_flags & ~(FL_TYPE_MASK|FL_SYM_LINK)) | new_flags;
  if (FL_IS_DIR (new_flags))
    {
      if (sub_dirs_vec == 0)
	walk_dir (flink);
      else if (!(new_flags & FL_SYM_LINK)) /* NEEDSWORK: optinally ignore? */
	dynvec_append (sub_dirs_vec, flink);
    }
  else
    {
      struct member_file *member;
#if HAVE_LINK
      member = maybe_get_member_file (flink, &st);
#else
      member = get_member_file (flink);
#endif
      if (member == 0)
	return;
#if 0
      member->mf_modify_time = st.st_mtime;
      member->mf_access_time = st.st_atime;
      if (member->mf_old_index < 0 || st.st_mtime > idh.idh_mod_time)
	member->mf_scan_index = 0;
#endif
    }
}

/****************************************************************************/
/* Serialize and write a file_link hierarchy. */

void
serialize_file_links (struct idhead *idhp)
{
  struct file_link **flinks_0;
  struct file_link **flinks;
  struct file_link **end;
  struct file_link **parents_0;
  struct file_link **parents;
  unsigned long parent_index = 0;
  
  flinks_0 = (struct file_link **) hash_dump (&idhp->idh_file_link_table,
					      0, file_link_qsort_compare);
  end = &flinks_0[idhp->idh_file_link_table.ht_fill];
  parents = parents_0 = MALLOC (struct file_link *, idhp->idh_file_link_table.ht_fill);
  for (flinks = flinks_0; flinks < end; flinks++)
    {
      struct file_link *flink = *flinks;
      if (!(flink->fl_flags & FL_USED))
	break;
      io_write (idhp->idh_FILE, flink->fl_name, 0, IO_TYPE_STR);
      io_write (idhp->idh_FILE, &flink->fl_flags, sizeof (flink->fl_flags), IO_TYPE_INT);
      io_write (idhp->idh_FILE, (IS_ROOT_FILE_LINK (flink)
				? &parent_index : &flink->fl_parent->fl_index),
		FL_PARENT_INDEX_BYTES, IO_TYPE_INT);
      *parents++ = flink->fl_parent; /* save parent link before clobbering */
      flink->fl_index = parent_index++;
    }
  /* restore parent links */
  for ((flinks = flinks_0), (parents = parents_0); flinks < end; flinks++)
    {
      struct file_link *flink = *flinks;
      if (!(flink->fl_flags & FL_USED))
	break;
      flink->fl_parent = *parents++;
    }
  free (parents_0);
  free (flinks_0);
  idhp->idh_file_links = parent_index;
  idhp->idh_files = idhp->idh_member_file_table.ht_fill;
}

/* Separate the wheat from the chaff.  Mark those file_links that are
   components in member files.  */

void
mark_member_file_links (struct idhead *idhp)
{
  struct member_file **members_0
    = (struct member_file **) hash_dump (&idhp->idh_member_file_table,
					 0, member_file_qsort_compare);
  struct member_file **end = &members_0[idhp->idh_member_file_table.ht_fill];
  struct member_file **members;
  int new_index = 0;

  for (members = members_0; members < end; members++)
    {
      struct member_file *member = *members;
      struct file_link *flink;
      member->mf_index = new_index++;
      for (flink = member->mf_link;
	   !(flink->fl_flags & FL_USED); flink = flink->fl_parent)
	flink->fl_flags |= FL_USED;
    }
  free (members_0);
}

/* Read and reconstruct a serialized file_link hierarchy.  */

struct file_link **
deserialize_file_links (struct idhead *idhp)
{
  struct file_link **flinks_0 = MALLOC (struct file_link *, idhp->idh_file_links);
  struct file_link **flinks = flinks_0;
  struct file_link **members_0 = MALLOC (struct file_link *, idhp->idh_files + 1);
  struct file_link **members = members_0;
  struct file_link *flink;
  struct file_link **slot;
  int i;

  for (i = 0; i < idhp->idh_file_links; i++)
    {
      unsigned long parent_index;
      int c;

      obstack_blank (&idhp->idh_file_link_obstack, offsetof (struct file_link, fl_name));
      if (obstack_room (&idhp->idh_file_link_obstack) >= idhp->idh_max_link)
	do
	  {
	    c = getc (idhp->idh_FILE);
	    obstack_1grow_fast (&idhp->idh_file_link_obstack, c);
	  }
	while (c);
      else
	do
	  {
	    c = getc (idhp->idh_FILE);
	    obstack_1grow (&idhp->idh_file_link_obstack, c);
	  }
	while (c);
      flink = (struct file_link *) obstack_finish (&idhp->idh_file_link_obstack);
      *flinks = flink;
      io_read (idhp->idh_FILE, &flink->fl_flags, sizeof (flink->fl_flags), IO_TYPE_INT);
      io_read (idhp->idh_FILE, &parent_index, FL_PARENT_INDEX_BYTES, IO_TYPE_INT);
      flink->fl_parent = flinks_0[parent_index];
      slot = (struct file_link **) hash_find_slot (&idhp->idh_file_link_table, flink);
      if (HASH_VACANT (*slot))
	hash_insert_at (&idhp->idh_file_link_table, flink, slot);
      else
	{
	  obstack_free (&idhp->idh_file_link_obstack, flink);
	  (*slot)->fl_flags = flink->fl_flags;
	  flink = *flinks = *slot;
	}
      flinks++;
      if (flink->fl_flags & FL_MEMBER)
	*members++ = flink;
    }
  free (flinks_0);
  *members = 0;
  return members_0;
}


#if HAVE_LINK      

/****************************************************************************/
/* Return a `member_file' for this `flink' *if* the filename matches
   some scan pattern, and no alias for the file takes precedence ([1]
   hard-links dominate symbolic-links; [2] for two hard-links: first
   come, first served).  */

struct member_file *
maybe_get_member_file (struct file_link *flink, struct stat *st)
{
  char buf[PATH_MAX];
  struct file_link *alias_link;
  struct member_file *member;
  struct member_file *alias_member = 0;

  member = get_member_file (flink);
  alias_link = find_alias_link (flink, st);
  if (alias_link)
    alias_member = find_member_file (alias_link);

  if (member && alias_member)
    {
      char alias_buf[PATH_MAX];
      int ancestry = symlink_ancestry (flink);
      int alias_ancestry = symlink_ancestry (alias_link);
      if (member->mf_lang_args != alias_member->mf_lang_args)
	error (0, 0, _("warning: `%s' and `%s' are the same file, but yield different scans!"),
	       absolute_path (buf, flink), absolute_path (alias_buf, alias_link));
      else if (alias_ancestry > ancestry)
	{
	  hash_delete (&idh.idh_member_file_table, member);
	  member->mf_link->fl_flags &= ~FL_MEMBER;
	  return 0;
	}
      else
	{
	  hash_delete (&idh.idh_member_file_table, alias_member);
	  alias_member->mf_link->fl_flags &= ~FL_MEMBER;
	}
    }
  return member;
}

/* Return a previously registered alias for `flink', if any.  */

struct file_link *
find_alias_link (struct file_link *flink, struct stat *st)      
{
  struct dev_ino *dev_ino;
  struct dev_ino **slot;

  dev_ino = (struct dev_ino *) obstack_alloc (&idh.idh_dev_ino_obstack, sizeof (struct dev_ino));
  dev_ino->di_dev = st->st_dev;
  dev_ino->di_ino = st->st_ino;
  slot = (struct dev_ino **) hash_find_slot (&idh.idh_dev_ino_table, dev_ino);
  if (HASH_VACANT (*slot))
    {
      dev_ino->di_link = flink;
      hash_insert_at (&idh.idh_dev_ino_table, dev_ino, slot);
      return 0;
    }
  else
    {
      obstack_free (&idh.idh_dev_ino_obstack, dev_ino);
      return (*slot)->di_link;
    }
}

/* Return the distance from `flink' to a symbolic-link ancestor
   directory.  PATH_MAX is considered an infinite distance (e.g.,
   there are no symlinks between `flink' and the root).  */

int
symlink_ancestry (struct file_link *flink)
{
  int ancestry = 0;
  while (!IS_ROOT_FILE_LINK (flink))
    {
      if (flink->fl_flags & FL_SYM_LINK)
	return ancestry;
      ancestry++;
      flink = flink->fl_parent;
    }
  return PATH_MAX;
}

#endif /* HAVE_LINK */

struct member_file *
get_member_file (struct file_link *flink)
{
  char buf[PATH_MAX];
  struct member_file *member;
  struct member_file **slot;
  struct lang_args const *args;

  args = get_lang_args (flink);
  if (args == 0)
    {
      DEBUG (("%s <IGNORE>\n", absolute_path (buf, flink)));
      return 0;
    }
  DEBUG (("%s <%s> <%s>\n", absolute_path (buf, flink),
	  args->la_language->lg_name, (args->la_args_string
				       ? args->la_args_string : "")));

  member = (struct member_file *) obstack_alloc (&idh.idh_member_file_obstack,
						 sizeof (struct member_file));
  member->mf_link = flink;
  slot = (struct member_file **) hash_find_slot (&idh.idh_member_file_table, member);
  if (HASH_VACANT (*slot))
    {
      member->mf_index = -1;
      hash_insert_at (&idh.idh_member_file_table, member, slot);
      flink->fl_flags |= FL_MEMBER;
    }
  else
    {
      obstack_free (&idh.idh_member_file_obstack, member);
#if 0
      if (member->mf_lang_args != args)
	{
	  error (0, 0, _("notice: scan parameters changed for `%s'"),
		 absolute_path (buf, flink));
	  member->mf_old_index = -1;
	}
#endif
      member = *slot;
    }
  member->mf_lang_args = args;
  return *slot;
}

struct member_file *
find_member_file (struct file_link const *flink)
{
  struct member_file key;
  struct member_file **slot;

  key.mf_link = (struct file_link *) flink;
  slot = (struct member_file **) hash_find_slot (&idh.idh_member_file_table, &key);
  if (HASH_VACANT (*slot))
    return 0;
  return *slot;
}

/* March down the list of lang_args, and return the first one whose
   pattern matches FLINK.  Return the matching lang_args, if a
   scanner exists for that language, otherwise return 0.  */

struct lang_args *
get_lang_args (struct file_link const *flink)
{
  struct lang_args *args = lang_args_list;
  
  while (args)
    {
      if (strchr (args->la_pattern, SLASH_CHAR))
	{
	  char buf[PATH_MAX];
	  absolute_path (buf, flink);
	  if (fnmatch (args->la_pattern, buf, MAYBE_FNM_CASEFOLD | FNM_FILE_NAME) == 0)
	    return (args->la_language ? args : 0);
	}
      else
	{
	  if (fnmatch (args->la_pattern, flink->fl_name, MAYBE_FNM_CASEFOLD) == 0)
	    return (args->la_language ? args : 0);
	}
      args = args->la_next;
    }
  return (lang_args_default->la_language ? lang_args_default : 0);
}

/****************************************************************************/
/* Convert a file name string to an absolute chain of `file_link's.  */

struct file_link *
parse_file_name (char *file_name, struct file_link *relative_dir_link)
{
  struct file_link *flink;

  if (IS_ABSOLUTE (file_name))
    {
#if HAVE_LINK
      flink = get_link_from_string (SLASH_STRING, 0);
#else
      flink = 0;
#endif
    }
  else if (relative_dir_link)
    flink = relative_dir_link;
  else if (current_dir_link)
    flink = current_dir_link;
  else
    flink = get_current_dir_link ();  

  for (;;)
    {
      char const* link_name = strtok (file_name, SLASH_STRING);
      if (link_name == 0)
	break;
      file_name = 0;
      if (*link_name == '\0' || IS_DOT (link_name))
	;
      else if (IS_DOT_DOT (link_name))
	flink = flink->fl_parent;
      else
	{
	  struct stat st;
	  flink = get_link_from_string (link_name, flink);
	  if (!flink->fl_flags)
	    flink->fl_flags = classify_link (flink, &st);
	}
    }
  return flink;
}

/* Return an absolute chain of `file_link's representing the current
   working directory.  */

struct file_link *
get_current_dir_link (void)
{
  struct file_link *dir_link;
  char *cwd_0;
  char *cwd;
  char *xcwd = 0;

  if (current_dir_link)
    return current_dir_link;

  cwd_0 = getenv ("PWD");
  if (cwd_0)
    cwd_0 = strdup (cwd_0);
  if (!same_as_dot (cwd_0))
    cwd_0 = xcwd = xgetcwd ();
  if (cwd_0 == 0)
    error (1, errno, _("can't get working directory"));
  cwd = cwd_0;
#if HAVE_LINK
  dir_link = get_link_from_string (SLASH_STRING, 0);
  dir_link->fl_flags = (dir_link->fl_flags & ~FL_TYPE_MASK) | FL_TYPE_DIR;
#else
  dir_link = 0;
#endif
  for (;;)
    {
      struct stat st;
      char const* link_name = strtok (cwd, SLASH_STRING);
      if (link_name == 0)
	break;
      cwd = 0;
      dir_link = get_link_from_string (link_name, dir_link);
      if (!dir_link->fl_flags)
	dir_link->fl_flags = classify_link (dir_link, &st);
    }
  chdir_to_link (dir_link);
  if (xcwd)
    free (xcwd);
  current_dir_link = dir_link;
  return dir_link;
}

static int
same_as_dot (char const *cwd)
{
  struct stat cwd_st;
  struct stat dot_st;

  if (cwd == 0 || *cwd != '/'
      || stat (cwd, &cwd_st) < 0
      || stat (".", &dot_st) < 0)
    return 0;
  return ((cwd_st.st_ino == dot_st.st_ino) && (cwd_st.st_dev == dot_st.st_dev));
}

/* Change the working directory to the directory represented by
   `dir_link'.  Choose the shortest path to the destination based on
   the cached value of the current directory.  */

int
chdir_to_link (struct file_link *dir_link)
{
  char to_buf[PATH_MAX];
  char from_buf[PATH_MAX];

  if (current_dir_link == dir_link)
    return 1;

  if (current_dir_link)
    maybe_relative_path (to_buf, dir_link, current_dir_link);
  else
    absolute_path (to_buf, dir_link);
  if (chdir (to_buf) < 0)
    {
      if (IS_ABSOLUTE (to_buf))
	error (0, errno, _("can't chdir to `%s'"), to_buf);
      else
	error (0, errno, _("can't chdir to `%s' from `%s'"), to_buf,
	       absolute_path (from_buf, current_dir_link));
      return 0;
    }
  else
    {
      current_dir_link = dir_link;
      return 1;
    }
}

/****************************************************************************/
/* Gather information about the link at `flink'.  If it's a good
   file or directory, return its mod-time and type.  */

int
classify_link (struct file_link *flink, struct stat *st)
{
  char buf[PATH_MAX];
  unsigned int flags = 0;

  if (!chdir_to_link (flink->fl_parent))
    return 0;

#ifdef S_IFLNK
  if (lstat (flink->fl_name, st) < 0)
    {
      error (0, errno, _("can't lstat `%s' from `%s'"), flink->fl_name, xgetcwd ());
      return 0;
    }
  if (S_ISLNK (st->st_mode))
    {
#endif
      if (stat (flink->fl_name, st) < 0)
	{
	  error (0, errno, _("can't stat `%s' from `%s'"), flink->fl_name, xgetcwd ());
	  return 0;
	}
#ifdef S_IFLNK
      flags |= FL_SYM_LINK;
    }
#endif
  if (S_ISDIR (st->st_mode))
    flags |= FL_TYPE_DIR;
  else if (S_ISREG (st->st_mode))
    flags |= FL_TYPE_FILE;
  else
    return 0;
  return flags;
}

/****************************************************************************/
/* Retrieve an existing flink; or if none exists, create one. */

struct file_link *
get_link_from_dirent (struct dirent *dirent, struct file_link *parent)
{
  struct file_link **slot;
  struct file_link *new_link;

  new_link = make_link_from_dirent (dirent, parent);
  slot = (struct file_link **) hash_find_slot (&idh.idh_file_link_table, new_link);
  if (HASH_VACANT (*slot))
    hash_insert_at (&idh.idh_file_link_table, new_link, slot);
  else
    obstack_free (&idh.idh_file_link_obstack, new_link);
  return *slot;
}

struct file_link *
get_link_from_string (char const *name, struct file_link *parent)
{
  struct file_link **slot;
  struct file_link *new_link;

  new_link = make_link_from_string (name, parent);
  slot = (struct file_link **) hash_find_slot (&idh.idh_file_link_table, new_link);
  if (HASH_VACANT (*slot))
    hash_insert_at (&idh.idh_file_link_table, new_link, slot);
  else
    obstack_free (&idh.idh_file_link_obstack, new_link);
  return *slot;
}

struct file_link *
make_link_from_dirent (struct dirent* dirent, struct file_link *parent)
{
  struct file_link *flink;

  flink = (struct file_link *) obstack_alloc (&idh.idh_file_link_obstack,
					      sizeof (struct file_link) + strlen (dirent->d_name));
  strcpy (flink->fl_name, dirent->d_name);
  flink->fl_parent = parent ? parent : flink;
  flink->fl_flags = 0;

  return flink;
}

struct file_link *
make_link_from_string (char const* name, struct file_link *parent)
{
  struct file_link *flink;

  flink = (struct file_link *) obstack_alloc (&idh.idh_file_link_obstack,
					      sizeof (struct file_link) + strlen (name));
  strcpy (flink->fl_name, name);
  flink->fl_parent = parent ? parent : flink;
  flink->fl_flags = 0;

  return flink;
}

/****************************************************************************/
/* Convert a `file_link' chain to a vector of component `file_link's,
   with the root at [0].  Return a pointer beyond the final component.  */

struct file_link const **
fill_link_vector (struct file_link const **vec_buf, struct file_link const *flink)
{
  vec_buf = fill_link_vector_1 (vec_buf, flink);
  *vec_buf = 0;
  return vec_buf;
}

struct file_link const **
fill_link_vector_1 (struct file_link const **vec_buf, struct file_link const *flink)
{
  if (!IS_ROOT_FILE_LINK (flink))
    vec_buf = fill_link_vector_1 (vec_buf, flink->fl_parent);
  *vec_buf++ = flink;
  return vec_buf;
}

/****************************************************************************/
/* Fill BUF_0 with a path to TO_LINK.  If a relative path from
   FROM_LINK is possible (i.e., no intervening symbolic-links) and
   shorter, return the relative path; otherwise, return an absolute
   path.  */

char const *
maybe_relative_path (char *buf_0, struct file_link const *to_link, struct file_link const *from_link)
{
  struct file_link const *to_link_vec_0[PATH_MAX/2];
  struct file_link const *from_link_vec_0[PATH_MAX/2];
  struct file_link const **to_link_vec = to_link_vec_0;
  struct file_link const **from_link_vec = from_link_vec_0;
  struct file_link const **from_link_end;
  struct file_link const **from_links;
  char *buf;
  int levels;

  if (from_link == 0)
    from_link = current_dir_link;

  /* Optimize common cases.  */
  if (to_link == from_link)
    return strcpy (buf_0, ".");
  else if (to_link->fl_parent == from_link)
    return strcpy (buf_0, to_link->fl_name);
  else if (from_link->fl_flags & FL_SYM_LINK)
    return absolute_path (buf_0, to_link);
  else if (to_link == from_link->fl_parent)
    return strcpy (buf_0, "..");
  else if (to_link->fl_parent == from_link->fl_parent)
    {
      strcpy (buf_0, DOT_DOT_SLASH);
      strcpy (&buf_0[3], to_link->fl_name);
      return buf_0;
    }

  from_link_end = fill_link_vector (from_link_vec, from_link);
  fill_link_vector (to_link_vec, to_link);
  while (*to_link_vec == *from_link_vec)
    {
      if (*to_link_vec == 0)
	return ".";
      to_link_vec++;
      from_link_vec++;
    }
  levels = from_link_end - from_link_vec;
  if (levels >= (from_link_vec - from_link_vec_0))
    return absolute_path (buf_0, to_link);
  for (from_links = from_link_vec; *from_links; from_links++)
    if ((*from_links)->fl_flags & FL_SYM_LINK)
      return absolute_path (buf_0, to_link);
  buf = fill_dot_dots (buf_0, levels);
  while (*to_link_vec)
    {
      strcpy (buf, (*to_link_vec)->fl_name);
      buf += strlen (buf);
      if ((*to_link_vec)->fl_name[0] != SLASH_CHAR && *++to_link_vec)
	*buf++ = SLASH_CHAR;
    }
  return buf_0;
}

/* Fill `buf' with sequences of "../" in order to ascend so many
   `levels' in a directory tree.  */

char *
fill_dot_dots (char *buf, int levels)
{
  while (levels--)
    {
      strcpy (buf, DOT_DOT_SLASH);
      buf += 3;
    }
  return buf;
}

/****************************************************************************/
/* Fill `buffer' with the absolute path to `flink'.  */

char *
absolute_path (char *buffer, struct file_link const *flink)
{
  char *end = absolute_path_1 (buffer, flink);
  /* Clip the trailing slash.  */
#if HAVE_LINK
  if (end > &buffer[1])
    end--;
#else
  if (end > &buffer[3])
    end--;
#endif
  *end = '\0';
  return buffer;
}

static char *
absolute_path_1 (char *buffer, struct file_link const *flink)
{
  char *end;
  if (IS_ROOT_FILE_LINK (flink))
    end = buffer;
  else
    end = absolute_path_1 (buffer, flink->fl_parent);
  strcpy (end, flink->fl_name);
  if (*end == SLASH_CHAR)
    end++;
  else
    {
      end += strlen (end);
      *end++ = SLASH_CHAR;
    }
  return end;
}

/****************************************************************************/
/* Hash stuff for `struct member_file'.  */

unsigned long
member_file_hash_1 (void const *key)
{
  return_ADDRESS_HASH_1 (((struct member_file const *) key)->mf_link);
}

unsigned long
member_file_hash_2 (void const *key)
{
  return_ADDRESS_HASH_2 (((struct member_file const *) key)->mf_link);
}

int
member_file_hash_compare (void const *x, void const *y)
{
  return_ADDRESS_COMPARE (((struct member_file const *) x)->mf_link,
			  ((struct member_file const *) y)->mf_link);
}

/* Collating sequence:
   - language.map index
   - mf_link: breadth-first, alphabetical */

int
member_file_qsort_compare (void const *x, void const *y)
{
  struct member_file const *mfx = *(struct member_file const **) x;
  struct member_file const *mfy = *(struct member_file const **) y;
  int result;

  INTEGER_COMPARE (mfx->mf_lang_args->la_index, mfy->mf_lang_args->la_index, result);
  if (result)
    return result;
  else
    {
      struct file_link const *flx = mfx->mf_link;
      struct file_link const *fly = mfy->mf_link;
      if (flx->fl_parent == fly->fl_parent)
	return strcmp (flx->fl_name, fly->fl_name);
      result = (links_depth (flx) - links_depth (fly));
      if (result)
	return result;
      while (flx->fl_parent != fly->fl_parent)
	{
	  flx = flx->fl_parent;
	  fly = fly->fl_parent;
	}
      return strcmp (flx->fl_name, fly->fl_name);
    }
}

/****************************************************************************/
/* Hash stuff for `struct file_link'.  */

unsigned long
file_link_hash_1 (void const *key)
{
  unsigned long result = 0;
  struct file_link const *parent = (IS_ROOT_FILE_LINK (((struct file_link const *) key))
				    ? 0 : ((struct file_link const *) key)->fl_parent);
  STRING_HASH_1 (((struct file_link const *) key)->fl_name, result);
  ADDRESS_HASH_1 (parent, result);
  return result;
}

unsigned long
file_link_hash_2 (void const *key)
{
  unsigned long result = 0;
  struct file_link const *parent = (IS_ROOT_FILE_LINK (((struct file_link const *) key))
				    ? 0 : ((struct file_link const *) key)->fl_parent);
  STRING_HASH_2 (((struct file_link const *) key)->fl_name, result);
  ADDRESS_HASH_2 (parent, result);
  return result;
}

int
file_link_hash_compare (void const *x, void const *y)
{
  int result;
  struct file_link const *x_parent = (IS_ROOT_FILE_LINK (((struct file_link const *) x))
				      ? 0 : ((struct file_link const *) x)->fl_parent);
  struct file_link const *y_parent = (IS_ROOT_FILE_LINK (((struct file_link const *) y))
				      ? 0 : ((struct file_link const *) y)->fl_parent);
  ADDRESS_COMPARE (x_parent, y_parent, result);
  if (result)
    return result;
  STRING_COMPARE (((struct file_link const *) x)->fl_name,
		  ((struct file_link const *) y)->fl_name, result);
  return result;
}

/* Collation sequence:
   - Used before unused.
   - Among used: breadth-first (dirs before files, parent dirs before children)
   - Among files: collate by mf_index.  */

int
file_link_qsort_compare (void const *x, void const *y)
{
  struct file_link const *flx = *(struct file_link const **) x;
  struct file_link const *fly = *(struct file_link const **) y;
  unsigned int x_flags = flx->fl_flags;
  unsigned int y_flags = fly->fl_flags;
  int result;

  result = (y_flags & FL_USED) - (x_flags & FL_USED);
  if (result)
    return result;
  if (!(x_flags & FL_USED))	/* If neither link is used, we don't care... */
    return 0;
  result = (y_flags & FL_TYPE_DIR) - (x_flags & FL_TYPE_DIR);
  if (result)
    return result;
  result = (y_flags & FL_TYPE_MASK) - (x_flags & FL_TYPE_MASK);
  if (result)
    return result;
  if (FL_IS_FILE (x_flags))
    {
      struct member_file *x_member = find_member_file (flx);
      struct member_file *y_member = find_member_file (fly);
      return x_member->mf_index - y_member->mf_index;
    }
  else
    {
      int x_depth = links_depth (flx);
      int y_depth = links_depth (fly);
      return (x_depth - y_depth);
    }
}

/* Count directory components between flink and its root.  */

int
links_depth (struct file_link const *flink)
{
  int depth = 0;
  while (!IS_ROOT_FILE_LINK (flink))
    {
      depth++;
      flink = flink->fl_parent;
    }
  return depth;
}

#if HAVE_LINK

/****************************************************************************/
/* Hash stuff for `struct dev_ino'.  */

unsigned long
dev_ino_hash_1 (void const *key)
{
  unsigned long result = 0;
  INTEGER_HASH_1 (((struct dev_ino const *) key)->di_dev, result);
  INTEGER_HASH_1 (((struct dev_ino const *) key)->di_ino, result);
  return result;
}

unsigned long
dev_ino_hash_2 (void const *key)
{
  unsigned long result = 0;
  INTEGER_HASH_2 (((struct dev_ino const *) key)->di_dev, result);
  INTEGER_HASH_2 (((struct dev_ino const *) key)->di_ino, result);
  return result;
}

int
dev_ino_hash_compare (void const *x, void const *y)
{
  int result;
  INTEGER_COMPARE (((struct dev_ino const *) x)->di_ino,
		   ((struct dev_ino const *) y)->di_ino, result);
  if (result)
    return result;
  INTEGER_COMPARE (((struct dev_ino const *) x)->di_dev,
		   ((struct dev_ino const *) y)->di_dev, result);
  return result;
}

#endif

/*******************************************************************/

void
init_idh_obstacks (struct idhead *idhp)
{
  obstack_init (&idhp->idh_member_file_obstack);
  obstack_init (&idhp->idh_file_link_obstack);
#if HAVE_LINK
  obstack_init (&idhp->idh_dev_ino_obstack);
#endif
}

void
init_idh_tables (struct idhead *idhp)
{
  hash_init (&idhp->idh_member_file_table, 16*1024,
	     member_file_hash_1, member_file_hash_2, member_file_hash_compare);
  hash_init (&idhp->idh_file_link_table, 16*1024,
	     file_link_hash_1, file_link_hash_2, file_link_hash_compare);
#if HAVE_LINK
  hash_init (&idhp->idh_dev_ino_table, 16*1024,
	     dev_ino_hash_1, dev_ino_hash_2, dev_ino_hash_compare);
#endif
}


#if TEST_IDWALK
/*******************************************************************/
/* Test program.  */

char const *program_name;
struct idhead idh;

void print_member_file __P((void const *item));

void
print_member_file (void const *item)
{
  char buf[PATH_MAX];
#define member ((struct member_file const *) item)
#if 1
  printf ("%s\n", maybe_relative_path (buf, member->mf_link, 0));
#else
  printf ("%ld %ld %s\n", member->mf_access_time, member->mf_modify_time,
	  maybe_relative_path (buf, member->mf_link, 0));
#endif
#undef member
}

void reset_walker (struct idhead *idhp);
void print_hash_stats (FILE *stream, struct idhead *idhp);

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

void
reset_walker (struct idhead *idhp)
{
  hash_delete_items (&idhp->idh_member_file_table);
  hash_delete_items (&idhp->idh_file_link_table);
#if HAVE_LINK
  hash_delete_items (&idhp->idh_dev_ino_table);
#endif
}

void
print_hash_stats (FILE *stream, struct idhead *idhp)
{
  fprintf (stream,   _("Link Table: ")); hash_print_stats (&idhp->idh_file_link_table, stream);
  fprintf (stream, _("\nFile Table: ")); hash_print_stats (&idhp->idh_member_file_table, stream);
#if HAVE_LINK
  fprintf (stream, _("\nDupl Table: ")); hash_print_stats (&idhp->idh_dev_ino_table, stream);
#endif
  fputc ('\n', stream);
}

int
main (int argc, char **argv)
{
  struct file_link *cwd_link;

  program_name = ((argc--, *argv++));

  init_idh_obstacks (&idh);
  init_idh_tables (&idh);

  parse_language_map (0);
  cwd_link = get_current_dir_link ();
  while (argc--)
    walk_flink (parse_file_name (*argv++, cwd_link), 0);
  
  chdir_to_link (cwd_link);

#if 0
  idh.idh_file_name = "idwalk.serial";
  idh.idh_FILE = fopen (idh.idh_file_name, "w+");
  if (idh.idh_FILE == 0)
    error (1, errno, _("can't open `%s' for writing"), idh.idh_file_name);

  printf (">>>>>>>>>>>>>>>> Serialize <<<<<<<<<<<<<<<<\n");
  hash_map (&idh.idh_member_file_table, print_member_file);
  printf (">>>>>>>>>>>>>>>> Serialize Stats <<<<<<<<<<<<<<<<\n");
  print_hash_stats (stdout, &idh);

  serialize_file_links (&idh);
  reset_walker (&idh);
  deserialize_file_links (&idh);

  printf (">>>>>>>>>>>>>>>> Deserialize <<<<<<<<<<<<<<<<\n");
  hash_map (&idh.idh_member_file_table, print_member_file);
  printf (">>>>>>>>>>>>>>>> Deserialize Stats <<<<<<<<<<<<<<<<\n");
  print_hash_stats (stdout, &idh);

  printf (">>>>>>>>>>>>>>>> End <<<<<<<<<<<<<<<<\n");
  fclose (idh.idh_FILE);
#endif
  return 0;
}

#endif

/*
  TODO:
  - stream I/O
  */
