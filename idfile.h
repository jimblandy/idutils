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
#include "hash.h"

#define	IDFILE	"ID"

struct idhead
{
  unsigned char idh_magic[2];
#define	IDH_MAGIC_0 ('I'|0x80)
#define	IDH_MAGIC_1 ('D'|0x80)
  unsigned char idh_version;
#define	IDH_VERSION	3
  unsigned short idh_flags;
#define IDH_COUNTS	0x0001	/* include occurrence counts for each token */
#define IDH_FOLLOW_SL	0x0002	/* follow symlinks to directories */
#define IDH_COMMENTS	0x0004	/* include tokens found in comments */
#define IDH_LOCALS	0x0008	/* include names of formal params & local vars */
#define IDH_DECL_DEFN_USE 0x0100 /* include decl/defn/use info */
#define IDH_L_R_VALUE	0x0200	/* include lvalue/rvalue info */
#define IDH_CALL_ER_EE	0x0400	/* include caller/callee relationship info */
  unsigned long idh_links;	/* total # of file name components */
  unsigned long idh_files;	/* total # of constituent source files */
  unsigned long idh_tokens;	/* total # of constituent tokens */
  /* idh_*_size: max buffer-sizes for ID file reading programs */
  unsigned long idh_buf_size;	/* # of bytes in longest entry */
  unsigned long idh_vec_size;	/* # of hits in longest entry */
  unsigned long idh_path_size;	/* # of bytes in longest file name path */
  /* idh_*_offset: ID file offsets for start of various sections */
  long idh_args_offset;		/* command-line options section */
  long idh_files_offset;	/* constituent file & directory names section */
  long idh_tokens_offset;	/* constituent tokens section */
  long idh_end_offset;		/* end of tokens section */
  /* */
  struct hash_table ia_link_table; /* all file and dir name name links */
  struct arg_file **ia_file_order; /* sequence in ID file */
  struct arg_file **ia_scan_order; /* sequence in summaries */
};

struct file_link
{
  struct file_link *fl_parent;
  unsigned char fl_flags;
#define FL_IS_ARG	0x01	/* is an explicit command-line argument */
#define FL_SYM_LINK	0x02	/* is a symlink (only used for dirs) */
#define FL_TYPE_MASK	0x10
# define FL_TYPE_DIR	0x00
# define FL_TYPE_FILE	0x10
  char fl_name[1];
};

struct arg_file
{
  struct file_link *af_name;
  short af_old_index;		/* order in extant ID file */
  short af_new_index;		/* order in new ID file */
  short af_scan_index;		/* order of scanning in summary */
};

#if HAVE_LINK

/* If the system supports filesystem links (e.g., any UN*X variant),
   we should detect file name aliases.  */

struct dev_ino
{
  dev_t di_dev;
  ino_t	di_ino;
  struct file_link *di_file_link;
};

extern struct hash_table dev_ino_table;

#endif

FILE *init_id_file __P((char const *id_file, struct idhead *idh));
int read_idhead __P((FILE *input_FILE, struct idhead *idh));
int write_idhead __P((FILE *input_FILE, struct idhead *idh));
int sizeof_idhead __P((void));

#endif /* not _idfile_h_ */
