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

static int io_idhead (FILE *fp, int (*io) (FILE *, void *, unsigned int, int), struct idhead *idh);
static int io_write (FILE *output_FILE, void *addr, unsigned int size, int is_int);
static int io_read (FILE *input_FILE, void *addr, unsigned int size, int is_int);
static int io_size (FILE *, void *, unsigned int size, int);

extern char *program_name;

/* init_id opens id_file, reads the header into idhp (and verifies the magic
 * number), then builds the id_args list holding the names of all the
 * files recorded in the database.
 */
FILE *
init_idfile (char const *id_file, struct idhead *idh, struct idarg **id_args)
{
  FILE *id_FILE;
  unsigned int i;
  char *strings;
  struct idarg *ida;

  id_FILE = fopen (id_file, "r");
  if (id_FILE == NULL)
    return NULL;

  read_idhead (id_FILE, idh);
  if (idh->idh_magic[0] != IDH_MAGIC_0 || idh->idh_magic[1] != IDH_MAGIC_1)
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
	case 4: /* This must be a literal 4.  Don't use sizeof (uintmin32_t)! */
	  *(uintmin32_t *)addr = getc (input_FILE);
	  *(uintmin32_t *)addr += getc (input_FILE) << 010;
	  *(uintmin32_t *)addr += getc (input_FILE) << 020;
	  *(uintmin32_t *)addr += getc (input_FILE) << 030;
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
	case 4: /* This must be a literal 4.  Don't use sizeof (uintmin32_t)! */
	  putc (*(uintmin32_t *)addr, output_FILE);
	  putc (*(uintmin32_t *)addr >> 010, output_FILE);
	  putc (*(uintmin32_t *)addr >> 020, output_FILE);
	  putc (*(uintmin32_t *)addr >> 030, output_FILE);
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
io_idhead (FILE *fp, int (*io) (FILE *, void *, unsigned int, int), struct idhead *idh)
{
  unsigned int size = 0;
  if (fp)
    fseek (fp, 0L, 0);
  size += io (fp, idh->idh_magic, 2, 0);
  size += io (fp, &idh->idh_pad_1, 1, 0);
  size += io (fp, &idh->idh_version, 1, 0);
  size += io (fp, &idh->idh_flags, 2, 1);
  size += io (fp, &idh->idh_args, 4, 1);
  size += io (fp, &idh->idh_paths, 4, 1);
  size += io (fp, &idh->idh_tokens, 4, 1);
  size += io (fp, &idh->idh_buf_size, 4, 1);
  size += io (fp, &idh->idh_vec_size, 4, 1);
  size += io (fp, &idh->idh_args_offset, 4, 1);
  size += io (fp, &idh->idh_tokens_offset, 4, 1);
  size += io (fp, &idh->idh_end_offset, 4, 1);
  return size;
}
