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
#include <obstack.h>

#include <config.h>
#include "alloc.h"
#include "idfile.h"
#include "strxtra.h"
#include "error.h"

typedef int (*iof_t) __P((FILE *, void *, unsigned int, int));
static int io_idhead __P((FILE *fp, iof_t iof, struct idhead *idh));
static int io_size __P((FILE *, void *, unsigned int size, int));
int fgets0 __P((char *buf0, int size, FILE *in_FILE));

extern char *program_name;

/* read_id_file opens the ID file, reads header fields into idh,
   verifies the magic number and version, and reads the constituent
   file names.  Any errors are considered fatal and cause an exit.  */

struct file_link **
read_id_file (char const *id_file_name, struct idhead *idh)
{
  struct file_link **flinkv = maybe_read_id_file (id_file_name, idh);
  if (flinkv)
    return flinkv;
  error (1, errno, _("can't open `%s'"), id_file_name);
  return NULL;
}

/* maybe_read_id_file does everything that read_id_file does, but is
   tolerant of errors opening the ID file, returning NULL in this case
   (this is called from mkid where an ID might or might not already
   exist).  All other errors are considered fatal.  */

struct file_link **
maybe_read_id_file (char const *id_file_name, struct idhead *idh)
{
  obstack_init (&idh->idh_file_link_obstack);
  idh->idh_FILE = fopen (id_file_name, "r");
  if (idh->idh_FILE == 0)
    return 0;

  read_idhead (idh);
  if (idh->idh_magic[0] != IDH_MAGIC_0 || idh->idh_magic[1] != IDH_MAGIC_1)
    error (1, 0, _("`%s' is not an ID file! (bad magic #)"), id_file_name);
  if (idh->idh_version != IDH_VERSION)
    error (1, 0, _("`%s' is version %d, but I only grok version %d"),
	   id_file_name, idh->idh_version, IDH_VERSION);

  fseek (idh->idh_FILE, idh->idh_flinks_offset, 0);
  return deserialize_file_links (idh);
}


int
read_idhead (struct idhead *idh)
{
  return io_idhead (idh->idh_FILE, io_read, idh);
}

int
write_idhead (struct idhead *idh)
{
  return io_idhead (idh->idh_FILE, io_write, idh);
}

int
sizeof_idhead ()
{
  return io_idhead (0, io_size, 0);
}

static int
io_size (FILE *ignore_FILE, void *ignore_addr, unsigned int size, int io_type)
{
  if (io_type == IO_TYPE_STR)
    error (0, 0, _("can't determine the io_size of a string!"));
  return size;
}

/* This is like fgets(3s), except that lines are delimited by NULs
   rather than newlines.  Also, we return the number of characters
   read rather than the address of buf0.  */

int
fgets0 (char *buf0, int size, FILE * in_FILE)
{
  char *buf;
  int c;
  char *end;

  buf = buf0;
  end = &buf[size];
  while ((c = getc (in_FILE)) > 0 && buf < end)
    *buf++ = c;
  *buf = '\0';
  return (buf - buf0);
}

int
io_read (FILE *input_FILE, void *addr, unsigned int size, int io_type)
{
  if (io_type == IO_TYPE_INT || size == 1)
    {
      switch (size)
	{
	case 4:
	  *(unsigned long *)addr = getc (input_FILE);
	  *(unsigned long *)addr += getc (input_FILE) << 010;
	  *(unsigned long *)addr += getc (input_FILE) << 020;
	  *(unsigned long *)addr += getc (input_FILE) << 030;
	  break;
	case 3:
	  *(unsigned long *)addr = getc (input_FILE);
	  *(unsigned long *)addr += getc (input_FILE) << 010;
	  *(unsigned long *)addr += getc (input_FILE) << 020;
	  break;
	case 2:
	  *(unsigned short *)addr = getc (input_FILE);
	  *(unsigned short *)addr += getc (input_FILE) << 010;
	  break;
	case 1:
	  *(unsigned char *)addr = getc (input_FILE);
	  break;
	default:
	  fprintf (stderr, _("unsupported size in io_write (): %d\n"), size);
	  abort ();
	}
    }
  else if (io_type == IO_TYPE_STR)
    fgets0 (addr, size, input_FILE);
  else if (io_type == IO_TYPE_FIX)
    fread (addr, size, 1, input_FILE);
  else
    error (0, 0, _("unknown I/O type: %d"), io_type);
  return size;
}

int
io_write (FILE *output_FILE, void *addr, unsigned int size, int io_type)
{
  if (io_type == IO_TYPE_INT || size == 1)
    {
      switch (size)
	{
	case 4:
	  putc (*(unsigned long *)addr, output_FILE);
	  putc (*(unsigned long *)addr >> 010, output_FILE);
	  putc (*(unsigned long *)addr >> 020, output_FILE);
	  putc (*(unsigned long *)addr >> 030, output_FILE);
	  break;
	case 3:
	  putc (*(unsigned long *)addr, output_FILE);
	  putc (*(unsigned long *)addr >> 010, output_FILE);
	  putc (*(unsigned long *)addr >> 020, output_FILE);
	  break;
	case 2:
	  putc (*(unsigned short *)addr, output_FILE);
	  putc (*(unsigned short *)addr >> 010, output_FILE);
	  break;
	case 1:
	  putc (*(unsigned char *)addr, output_FILE);
	  break;
	default:
	  fprintf (stderr, _("unsupported size in io_write (): %d\n"), size);
	  abort ();
	}
    }
  else if (io_type == IO_TYPE_STR) {
    fputs (addr, output_FILE);
    putc ('\0', output_FILE);
  } else if (io_type == IO_TYPE_FIX)
    fwrite (addr, size, 1, output_FILE);
  else
    error (0, 0, _("unknown I/O type: %d"), io_type);
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
  size += iof (fp, idh->idh_magic, 2, IO_TYPE_FIX);
  size += iof (fp, &pad, 1, IO_TYPE_FIX);
  size += iof (fp, &idh->idh_version, 1, IO_TYPE_FIX);
  size += iof (fp, &idh->idh_flags, 2, IO_TYPE_INT);
  size += iof (fp, &idh->idh_file_links, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_files, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_tokens, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_buf_size, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_vec_size, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_tokens_offset, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_flinks_offset, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_end_offset, 4, IO_TYPE_INT);
  size += iof (fp, &idh->idh_max_link, 2, IO_TYPE_INT);
  size += iof (fp, &idh->idh_max_path, 2, IO_TYPE_INT);
  return size;
}
