/* bitops.h -- defs for interface to bitops.c, plus bit-vector macros
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

#ifndef _bitops_h_
#define _bitops_h_

#define	BITTST(ba, bn)	((ba)[(bn) >> 3] &  (1 << ((bn) & 0x07)))
#define	BITSET(ba, bn)	((ba)[(bn) >> 3] |= (1 << ((bn) & 0x07)))
#define	BITCLR(ba, bn)	((ba)[(bn) >> 3] &=~(1 << ((bn) & 0x07)))
#define	BITAND(ba, bn)	((ba)[(bn) >> 3] &= (1 << ((bn) & 0x07)))
#define	BITXOR(ba, bn)	((ba)[(bn) >> 3] ^= (1 << ((bn) & 0x07)))

int vec_to_bits (char *bit_array, char *vec, int size);
int bits_to_vec (char *vec, char *bit_array, int bit_count, int size);

#endif /* _bitops_h_ */
