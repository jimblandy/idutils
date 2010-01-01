/* dynvec.h -- declarations for dynamically growable vectors
   Copyright (C) 1995, 2007-2010 Free Software Foundation, Inc.
   Written by Greg McGary <gkm@gnu.ai.mit.edu>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _dynvec_h_
#define _dynvec_h_

struct dynvec
{
  void **dv_vec;
  int dv_capacity;
  int dv_fill;
};

extern struct dynvec *make_dynvec (int n);
extern void dynvec_free (struct dynvec *dv);
extern void dynvec_freeze (struct dynvec *dv);
extern void dynvec_append (struct dynvec *dv, void *element);

#endif /* not _dynvec_h_ */
