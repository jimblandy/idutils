/* idfile.c -- read & write mkid database file header
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

#include <stdio.h>
#include <string.h>

#include <config.h>
#include "alloc.h"
#include "idfile.h"
#include "strxtra.h"

typedef int (*iof_t) __P((FILE *, void *, unsigned int, int));
static int io_idhead __P((FILE *fp, iof_t iof, struct idhead *idh));
static int io_write __P((FILE *output_FILE, void *addr, unsigned int size, int is_int));
static int io_read __P((FILE *input_FILE, void *addr, unsigned int size, int is_int));
static int io_size __P((FILE *, void *, unsigned int size, int));

extern char *program_name;

/* init_id_file opens the ID file, reads header fields into idh,
   verifies the magic number and version, and reads the constituent
   file names.  Any errors are considered fatal and cause an exit.  */

FILE *
init_id_file (char const *id_file_name, struct idhead *idh)
{
  FILE *id_FILE = maybe_init_id_file (id_file_name, idh);
  if (id_FILE)
    return id_FILE;
  error (1, errno, "Can't open `%s'", id_file_name);
  return NULL;
}

/* maybe_init_id_file does everything that init_id_file does, but is
   tolerant of errors opening the ID file, returning NULL in this case
   (this is called from mkid where an ID might or might not already
   exist).  All other errors are considered fatal.  */

FILE *
maybe_init_id_file (char const *id_file_name, struct idhead *idh)
{
  FILE *id_FILE;
  unsigned int i;
  char *strings;
  struct idarg *ida;

  id_FILE = fopen (id_file_name, "r");
  if (id_FILE == NULL)
    return NULL;

  read_idhead (id_FILE, idh);
  if (idh->idh_magic[0] != IDH_MAGIC_0 || idh->idh_magic[1] != IDH_MAGIC_1)
    error (1, 0, "`%s' is not an ID file! (bad magic #)", id_file_name);
  if (idh->idh_version != IDH_VERSION)
    error (1, 0, "`%s' is version %d, but I only grok version %d",
	   id_file_name, idh->idh_version, IDH_VERSION);

  fseek (id_FILE, idh->idh_args_offset, 0);
  /* NEEDSWORK */
  fseek (id_FILE, idh->idh_files_offset, 0);
  
  i = idh->idh_tokens_offset - idh->idh_args_offset;
  strings = malloc (i);
  fread (strings, i, 1, id_FILE);
  ida = *id_args =  CALLOC (struct idarg, idh->idh_files);
  for (i = 0; i < idh->idh_files; i++)
    {
      while (*strings == '+' || *strings == '-')
	{
	  while (*strings++)
	    ;
	}
      ida->ida_flags = 0;
      ida->ida_arg = strings;
      ida->ida_next = ida + 1;
      ida->ida_index = i;
      ida++;
      while (*strings++)
	;
    }
  (--ida)->ida_next = NULL;
  return id_FILE;
}


unsigned long
file_link_hash_1 (void const *key)
{
  unsigned long result = 0;
  ADDRESS_HASH_1 (((struct file_link const *) key)->fl_parent, result);
  STRING_HASH_1 (((struct file_link const *) key)->fl_name, result);
  return result;
}

unsigned long
file_link_hash_2 (void const *key)
{
  unsigned long result = 0;
  ADDRESS_HASH_2 (((struct file_link const *) key)->fl_parent, result);
  STRING_HASH_2 (((struct file_link const *) key)->fl_name, result);
  return result;
}

int
file_link_hash_cmp (void const *x, void const *y)
{
  int result;
  ADDRESS_CMP (((struct file_link const *) x)->fl_parent,
	       ((struct file_link const *) y)->fl_parent, result);
  if (result)
    return result;
  STRING_CMP (((struct file_link const *) x)->fl_name,
	      ((struct file_link const *) y)->fl_name, result);
  return result;
}


int
read_idhead (FILE *input_FILE, struct idhead *idh)
{
  return io_idhead (input_FILE, io_read, idh);
}

int
write_idhead (FILE *input_FILE, struct idhead *idh)
{
  return io_idhead (input_FILE, io_write, idh);
}

int
sizeof_idhead ()
{
  return io_idhead (0, io_size, 0);
}

static int
io_size (FILE *ignore_FILE, void *ignore_addr, unsigned int size, int ignore_int)
{
  return size;
}

static int
io_read (FILE *input_FILE, void *addr, unsigned int size, int is_int)
{
  if (is_int)
    {
      switch (size)
	{
	case 4: /* This must be a literal 4.  Don't use sizeof (unsigned long)! */
	  *(unsigned long *)addr = getc (input_FILE);
	  *(unsigned long *)addr += getc (input_FILE) << 010;
	  *(unsigned long *)addr += getc (input_FILE) << 020;
	  *(unsigned long *)addr += getc (input_FILE) << 030;
	  break;
	case 2:
	  *(unsigned short *)addr = getc (input_FILE);
	  *(unsigned short *)addr += getc (input_FILE) << 010;
	  break;
	case 1:
	  *(unsigned char *)addr = getc (input_FILE);
	  break;
	default:
	  fprintf (stderr, "Unsupported size in io_write (): %d\n", size);
	  abort ();
	}
    }
  else if (size > 1)
    fread (addr, size, 1, input_FILE);
  else
    *(char *)addr = getc (input_FILE);
  return size;
}

static int
io_write (FILE *output_FILE, void *addr, unsigned int size, int is_int)
{
  if (is_int)
    {
      switch (size)
	{
	case 4: /* This must be a literal 4.  Don't use sizeof (unsigned long)! */
	  putc (*(unsigned long *)addr, output_FILE);
	  putc (*(unsigned long *)addr >> 010, output_FILE);
	  putc (*(unsigned long *)addr >> 020, output_FILE);
	  putc (*(unsigned long *)addr >> 030, output_FILE);
	  break;
	case 2:
	  putc (*(unsigned short *)addr, output_FILE);
	  putc (*(unsigned short *)addr >> 010, output_FILE);
	  break;
	case 1:
	  putc (*(unsigned char *)addr, output_FILE);
	  break;
	default:
	  fprintf (stderr, "Unsupported size in io_write (): %d\n", size);
	  abort ();
	}
    }
  else if (size > 1)
    fwrite (addr, size, 1, output_FILE);
  else
    putc (*(char *)addr, output_FILE);
  return size;
}

/* The sizes of the fields must be hard-coded.  They aren't
   necessarily the sizes of the struct members, because some
   architectures don't have any way to declare 4-byte integers
   (e.g., Cray) */

static int
io_idhead (FILE *fp, iof_t iof, struct idhead *idh)
{
  unsigned int size = 0;
  unsigned char pad = 0;
  if (fp)
    fseek (fp, 0L, 0);
  size += iof (fp, idh->idh_magic, 2, 0);
  size += iof (fp, &pad, 1, 0);
  size += iof (fp, &idh->idh_version, 1, 0);
  size += iof (fp, &idh->idh_flags, 2, 1);
  size += iof (fp, &idh->idh_links, 4, 1);
  size += iof (fp, &idh->idh_files, 4, 1);
  size += iof (fp, &idh->idh_tokens, 4, 1);
  size += iof (fp, &idh->idh_buf_size, 4, 1);
  size += iof (fp, &idh->idh_vec_size, 4, 1);
  size += iof (fp, &idh->idh_args_offset, 4, 1);
  size += iof (fp, &idh->idh_tokens_offset, 4, 1);
  size += iof (fp, &idh->idh_end_offset, 4, 1);
  return size;
}
