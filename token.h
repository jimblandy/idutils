/* token.h -- defs for interface to token.c
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

#ifndef _token_h_
#define _token_h_

/* token flags (struct token is in mkid.c) */
#define TOK_VECTOR	0x01	/* 1 = hits are stored as a vector
				   0 = hits are stored as a 8-way tree of bits
				   mkid chooses whichever is more compact.
				   vector is more compact for tokens with few hits */
#define TOK_NUMBER	0x02	/* occurs as a number */
#define TOK_NAME	0x04	/* occurs as a name */
#define TOK_STRING	0x08	/* occurs in a string */
#define TOK_LITERAL	0x10	/* occurs as a literal */
#define TOK_COMMENT	0x20	/* occurs in a comment */
#define TOK_UNUSED_1	0x40
#define TOK_SHORT_COUNT	0x80	/* count is two bytes */

#define tok_string(buf) (buf)
unsigned int tok_flags (char const *buf);
unsigned short tok_count (char const *buf);
unsigned char const *tok_hits_addr (char const *buf);

#endif /* _token_h_ */
