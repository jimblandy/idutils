/* idarg.h -- defs for internal form of command-line arguments
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

#ifndef _idarg_h_
#define _idarg_h_

struct idarg
{
  struct idarg *ida_next;
  char *ida_arg;
  int ida_index;
  char ida_flags;
#define	IDA_RELATIVE	0x01	/* file name is now relative (lid) */
#define	IDA_SCAN_ME	0x01	/* file should be scanned (mkid) */
#define	IDA_PREFIX_US	0x02	/* file has names with prefixed underscores */
};

#endif /* not _idarg_h_ */
