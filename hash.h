/* hash.h -- decls for hash table
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

#ifndef _hash_h_
#define _hash_h_

typedef unsigned long (*hash_t) __P((void const *key));
typedef int (*hash_cmp_t) __P((void const *x, void const *y));

struct hash_table 
{
  void **ht_vec;
  unsigned long ht_size;	/* total number of slots (power of 2) */
  unsigned long ht_capacity;	/* usable slots, limited by loading-factor */
  unsigned long ht_fill;	/* items in table */
  unsigned long ht_probes;	/* number of comparisons */
  unsigned long ht_lookups;	/* number of queries */
  unsigned int ht_rehashes;	/* number of times we've expanded table */
  hash_t ht_hash_1;		/* primary hash function */
  hash_t ht_hash_2;		/* secondary hash function */
  hash_cmp_t ht_compare;	/* comparison function */
};

void hash_init __P((struct hash_table* ht, long size,
		    hash_t hash_1, hash_t hash_2, hash_cmp_t hash_cmp));
void rehash __P((struct hash_table* ht));
void **hash_lookup __P((struct hash_table* ht, void const *key));


/* hash and comparison macros for string keys. */

#define STRING_HASH_1(_key_, _result_) { \
  unsigned char const *kk = (unsigned char const *) (_key_) - 1; \
  while (*++kk) \
    (_result_) += (*kk << (kk[1] & 0xf)); \
} while (0)
#define return_STRING_HASH_1(_key_) { \
  unsigned long result = 0; \
  STRING_HASH_1 ((_key_), result); \
  return result; \
} while (0)

#define STRING_HASH_2(_key_, _result_) { \
  unsigned char const *kk = (unsigned char const *) (_key_) - 1; \
  while (*++kk) \
    (_result_) += (*kk << (kk[1] & 0x7)); \
} while (0)
#define return_STRING_HASH_2(_key_) { \
  unsigned long result = 0; \
  STRING_HASH_2 ((_key_), result); \
  return result; \
} while (0)

#define STRING_COMPARE(_x_, _y_, _result_) { \
  unsigned char const *xx = (unsigned char const *) (_x_) - 1; \
  unsigned char const *yy = (unsigned char const *) (_y_) - 1; \
  do { \
    if (*++xx == '\0') { \
      yy++; \
      break; \
    } \
  } while (*xx == *++yy); \
  (_result_) = *xx - *yy; \
} while (0)
#define return_STRING_COMPARE(_x_, _y_) { \
  int result; \
  STRING_COMPARE (_x_, _y_, result); \
  return result; \
} while (0)

/* hash and comparison macros for integer keys. */

#define INTEGER_HASH_1(_key_, _result_) { \
  (_result_) = ((unsigned long)(_key_)); \
} while (0)
#define return_INTEGER_HASH_1(_key_) { \
  unsigned long result = 0; \
  INTEGER_HASH_1 ((_key_), result); \
  return result; \
} while (0)

#define INTEGER_HASH_2(_key_, _result_) { \
  (_result_) = ~((unsigned long)(_key_)); \
} while (0)
#define return_INTEGER_HASH_2(_key_) { \
  unsigned long result = 0; \
  INTEGER_HASH_2 ((_key_), result); \
  return result; \
} while (0)

#define INTEGER_COMPARE(_x_, _y_, _result_) { \
  (_result_) = _x_ - _y_; \
} while (0)
#define return_INTEGER_COMPARE(_x_, _y_) { \
  int result; \
  INTEGER_COMPARE (_x_, _y_, result); \
  return result; \
} while (0)

/* hash and comparison macros for address keys. */

#define ADDRESS_HASH_1(_key_, _result_) INTEGER_HASH_1 (((unsigned long)(_key_)) >> 3, (_result_))
#define ADDRESS_HASH_2(_key_, _result_) INTEGER_HASH_2 (((unsigned long)(_key_)) >> 3, (_result_))
#define ADDRESS_COMPARE(_x_, _y_, _result_) INTEGER_COMPARE ((_x_), (_y_), (_result_))
#define return_ADDRESS_HASH_1(_key_) return_INTEGER_HASH_1 (((unsigned long)(_key_)) >> 3)
#define return_ADDRESS_HASH_2(_key_) return_INTEGER_HASH_2 (((unsigned long)(_key_)) >> 3)
#define return_ADDRESS_COMPARE(_x_, _y_) return_INTEGER_COMPARE ((_x_), (_y_))

#endif /* not _hash_h_ */
