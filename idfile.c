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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "idfile.h"
#include "idarg.h"
#include "strxtra.h"

static int io_idhead (FILE *fp, int (*io) (FILE *, void *, int, int), struct idhead *idh);
static int io_write (FILE *output_FILE, void *addr, int size, int is_int);
static int io_read (FILE *input_FILE, void *addr, int size, int is_int);
static int io_size (FILE *, void *, int size, int);

extern char *program_name;

/* init_id opens id_file, reads the header into idhp (and verifies the magic
 * number), then builds the id_args list holding the names of all the
 * files recorded in the database.
 */
FILE *
init_idfile (char const *id_file, struct idhead *idh, struct idarg **id_args)
{
  FILE *id_FILE;
  int i;
  char *strings;
  struct idarg *ida;

  id_FILE = fopen (id_file, "r");
  if (id_FILE == NULL)
    return NULL;

  read_idhead (id_FILE, idh);
  if (!strnequ (idh->idh_magic, IDH_MAGIC, sizeof (idh->idh_magic)))
    {
      fprintf (stderr, "%s: Not an id file: `%s'\n", program_name, id_file);
      exit (1);
    }
  if (idh->idh_version != IDH_VERSION)
    {
      fprintf (stderr, "%s: ID version mismatch (want: %d, got: %d)\n", program_name, IDH_VERSION, idh->idh_version);
      exit (1);
    }

  fseek (id_FILE, idh->idh_args_offset, 0);
  strings = malloc (i = idh->idh_tokens_offset - idh->idh_args_offset);
  fread (strings, i, 1, id_FILE);
  ida = *id_args =  CALLOC (struct idarg, idh->idh_paths);
  for (i = 0; i < idh->idh_args; i++)
    {
      if (*strings != '+' && *strings != '-')
	{
	  ida->ida_flags = 0;
	  ida->ida_arg = strings;
	  ida->ida_next = ida + 1;
	  ida->ida_index = i;
	  ida++;
	}
      while (*strings++)
	;
    }
  (--ida)->ida_next = NULL;
  return id_FILE;
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
io_size (FILE *ignore_FILE, void *ignore_addr, int size, int ignore_int)
{
  return size;
}

static int
io_read (FILE *input_FILE, void *addr, int size, int is_int)
{
  if (is_int)
    {
      switch (size)
	{
	case sizeof (long):
	  *(long *)addr = getc (input_FILE);
	  *(long *)addr += getc (input_FILE) << 010;
	  *(long *)addr += getc (input_FILE) << 020;
	  *(long *)addr += getc (input_FILE) << 030;
	  break;
	case sizeof (short):
	  *(short *)addr = getc (input_FILE);
	  *(short *)addr += getc (input_FILE) << 010;
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
io_write (FILE *output_FILE, void *addr, int size, int is_int)
{
  if (is_int)
    {
      switch (size)
	{
	case sizeof (long):
	  putc (*(long *)addr, output_FILE);
	  putc (*(long *)addr >> 010, output_FILE);
	  putc (*(long *)addr >> 020, output_FILE);
	  putc (*(long *)addr >> 030, output_FILE);
	  break;
	case sizeof (short):
	  putc (*(short *)addr, output_FILE);
	  putc (*(short *)addr >> 010, output_FILE);
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

static int
io_idhead (FILE *fp, int (*io) (FILE *, void *, int, int), struct idhead *idh)
{
  int size = 0;
  fseek (fp, 0L, 0);
  size += io (fp, idh->idh_magic, sizeof (idh->idh_magic), 0);
  size += io (fp, &idh->idh_pad_1, sizeof (idh->idh_pad_1), 0);
  size += io (fp, &idh->idh_version, sizeof (idh->idh_version), 0);
  size += io (fp, &idh->idh_flags, sizeof (idh->idh_flags), 1);
  size += io (fp, &idh->idh_args, sizeof (idh->idh_args), 1);
  size += io (fp, &idh->idh_paths, sizeof (idh->idh_paths), 1);
  size += io (fp, &idh->idh_tokens, sizeof (idh->idh_tokens), 1);
  size += io (fp, &idh->idh_buf_size, sizeof (idh->idh_buf_size), 1);
  size += io (fp, &idh->idh_vec_size, sizeof (idh->idh_vec_size), 1);
  size += io (fp, &idh->idh_args_offset, sizeof (idh->idh_args_offset), 1);
  size += io (fp, &idh->idh_tokens_offset, sizeof (idh->idh_tokens_offset), 1);
  size += io (fp, &idh->idh_end_offset, sizeof (idh->idh_end_offset), 1);
  return size;
}

