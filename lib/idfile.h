/* idfile.h -- decls for ID file header and constituent file names
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

#ifndef _idfile_h_
#define _idfile_h_ 1

#include <sys/types.h>
#include <stdio.h>
#include <obstack.h>
#include "hash.h"

/* The ID file header is the nexus of all ID file information.  This
   is an in-core structure, only some of which is read/written to disk.  */

struct idhead
{
  unsigned char idh_magic[2];
#define	IDH_MAGIC_0 ('I'|0x80)
#define	IDH_MAGIC_1 ('D'|0x80)
  unsigned char idh_version;
#define	IDH_VERSION	4
  unsigned short idh_flags;
#define IDH_COUNTS	(1<<0)	/* include occurrence counts for each token */
#define IDH_FOLLOW_SL	(1<<1)	/* follow symlinks to directories */
#define IDH_COMMENTS	(1<<2)	/* include tokens found in comments */
#define IDH_LOCALS	(1<<3)	/* include names of formal params & local vars */
#define IDH_DECL_DEFN_USE (1<<4) /* include decl/defn/use info */
#define IDH_L_R_VALUE	(1<<5)	/* include lvalue/rvalue info */
#define IDH_CALL_ER_EE	(1<<6)	/* include caller/callee relationship info */
  unsigned long idh_file_links;	/* total # of file links */
  unsigned long idh_files;	/* total # of constituent source files */
  unsigned long idh_tokens;	/* total # of constituent tokens */
  /* idh_*_size: max buffer-sizes for ID file reading programs */
  unsigned long idh_buf_size;	/* # of bytes in longest entry */
  unsigned long idh_vec_size;	/* # of hits in longest entry */
  /* idh_*_offset: ID file offsets for start of various sections */
  long idh_tokens_offset;	/* constituent tokens section */
  long idh_flinks_offset;	/* constituent file & directory names section */
  long idh_end_offset;		/* end of tokens section */
  unsigned short idh_max_link;	/* longest file name component */
  unsigned short idh_max_path;	/* largest # of file name components */

  /* The following are run-time variables and are not stored on disk */
  struct hash_table idh_member_file_table;
  struct hash_table idh_file_link_table;
#if HAVE_LINK
  struct hash_table idh_dev_ino_table; /* for detecting file name aliases */
#endif
  struct obstack idh_member_file_obstack;
  struct obstack idh_file_link_obstack;
#if HAVE_LINK
  struct obstack idh_dev_ino_obstack;
#endif
  char *idh_file_name;
  FILE *idh_FILE;
#if 0
  time_t idh_mod_time;
  struct arg_file **idh_file_order; /* sequence in ID file */
  struct arg_file **idh_scan_order; /* sequence in summaries */
#endif
};

/* A file_link represents a single component (file or directory) in a
   file name.  It has a name, a parent file_link and some flags.  */

struct file_link
{
  union {
    struct file_link *u_parent;
#define fl_parent fl_u.u_parent
    unsigned long u_index;
#define fl_index fl_u.u_index
#define FL_PARENT_INDEX_BYTES 3
#define IS_ROOT_FILE_LINK(flink) ((flink)->fl_parent == (flink))
  } fl_u;
  unsigned char fl_flags;
#define FL_CMD_LINE_ARG	(1<<0)
#define FL_USED		(1<<1)
#define FL_MEMBER	(1<<2)	/* has a corresponding member_file entry */
#define FL_SCAN_ME	(1<<3)
#define FL_SYM_LINK	(1<<4)
#define FL_TYPE_MASK	(FL_TYPE_DIR|FL_TYPE_FILE)
# define FL_TYPE_DIR	(1<<5)
# define FL_IS_DIR(_f_) (((_f_) & FL_TYPE_MASK) == FL_TYPE_DIR)
# define FL_TYPE_FILE	(1<<6)
# define FL_IS_FILE(_f_) (((_f_) & FL_TYPE_MASK) == FL_TYPE_FILE)
  char fl_name[1];
};

/* A member_file represents a source file that is treated by mkid.  */

struct member_file
{
  struct file_link *mf_link;
  struct lang_args const *mf_lang_args;
time_t mf_modify_time;
time_t mf_access_time;
  short mf_index;		/* order in ID file */
};

#if HAVE_LINK

/* On systems that support multiple names for a single file (via hard
  and/or soft links), dev_ino records information needed to detect
  such aliasing.  */

struct dev_ino
{
  dev_t di_dev;
  ino_t	di_ino;
  struct file_link *di_link;
};

extern struct hash_table dev_ino_table;

#endif

extern struct idhead idh;

extern struct file_link **read_id_file __P((char const *id_file_name, struct idhead *idhp));
extern struct file_link **maybe_read_id_file __P((char const *id_file_name, struct idhead *idhp));
extern int read_idhead __P((struct idhead *idhp));
extern int write_idhead __P((struct idhead *idhp));
extern int sizeof_idhead __P((void));
extern void init_idh_obstacks __P((struct idhead *idhp));
extern void init_idh_tables __P((struct idhead *idhp));

#define IO_TYPE_INT	0	/* integer */
#define IO_TYPE_STR	1	/* NUL terminated string */
#define IO_TYPE_FIX	2	/* fix-sized */

extern int io_write __P((FILE *output_FILE, void *addr, unsigned int size, int io_type));
extern int io_read __P((FILE *input_FILE, void *addr, unsigned int size, int io_type));
extern struct file_link *get_current_dir_link __P((void));

extern struct file_link **deserialize_file_links __P((struct idhead *idhp));
extern void mark_member_file_links __P((struct idhead *idhp));
extern int member_file_qsort_compare __P((void const *x, void const *y));
extern void serialize_file_links __P((struct idhead *idhp));
extern struct file_link *parse_file_name __P((char *file_name,
					      struct file_link *relative_dir_link));

#endif /* not _idfile_h_ */
