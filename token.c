/* token.c -- misc. access functions for mkid database tokens
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

#include <config.h>
#include "token.h"

unsigned int
tok_flags (char const *buf)
{
  return *(unsigned char const *)&buf[strlen (buf) + 1];
}

#define TOK_COUNT_ADDR(buf) ((unsigned char const *)(TOK_FLAGS_ADDR (buf) + 1))
#define TOK_HITS_ADDR(buf) ((unsigned char const *)(TOK_COUNT_ADDR (buf) + 2))

unsigned short
tok_count (char const *buf)
{
  unsigned char const *flags = (unsigned char const *)&buf[strlen (buf) + 1];
  unsigned char const *addr = flags + 1;
  unsigned short count = *addr;
  if (*flags & TOK_SHORT_COUNT)
    count += (*++addr << 8);
  return count;
}

unsigned char const *
tok_hits_addr (char const *buf)
{
  unsigned char const *flags = (unsigned char const *)&buf[strlen (buf) + 1];
  unsigned char const *addr = flags + 2;
  if (*flags & TOK_SHORT_COUNT)
    addr++;
  return addr;
}
