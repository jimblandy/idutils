/* bitops.c -- Bit-vector manipulation for mkid
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

#include <config.h>
#include "bitops.h"

static int str_to_int __P((char *bufp, int size));
static char *int_to_str __P((int i, int size));

int
vec_to_bits (char *bit_array, char *vec, int size)
{
  int i;
  int count;

  for (count = 0; (*vec & 0xff) != 0xff; count++)
    {
      i = str_to_int (vec, size);
      BITSET (bit_array, i);
      vec += size;
    }
  return count;
}

int
bits_to_vec (char *vec, char *bit_array, int bit_count, int size)
{
  char *element;
  int i;
  int count;

  for (count = i = 0; i < bit_count; i++)
    {
      if (!BITTST (bit_array, i))
	continue;
      element = int_to_str (i, size);
      switch (size)
	{
	case 4:
	  *vec++ = *element++;
	case 3:
	  *vec++ = *element++;
	case 2:
	  *vec++ = *element++;
	case 1:
	  *vec++ = *element++;
	}
      count++;
    }
  *vec++ = 0xff;

  return count;
}

/* NEEDSWORK: ENDIAN */

static char *
int_to_str (int i, int size)
{
  static char buf0[4];
  char *bufp = &buf0[size];

  switch (size)
    {
    case 4:
      *--bufp = (i & 0xff);
      i >>= 8;
    case 3:
      *--bufp = (i & 0xff);
      i >>= 8;
    case 2:
      *--bufp = (i & 0xff);
      i >>= 8;
    case 1:
      *--bufp = (i & 0xff);
    }
  return buf0;
}

static int
str_to_int (char *bufp, int size)
{
  int i = 0;

  bufp--;
  switch (size)
    {
    case 4:
      i |= (*++bufp & 0xff);
      i <<= 8;
    case 3:
      i |= (*++bufp & 0xff);
      i <<= 8;
    case 2:
      i |= (*++bufp & 0xff);
      i <<= 8;
    case 1:
      i |= (*++bufp & 0xff);
    }
  return i;
}
