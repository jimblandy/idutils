/* xnls.h -- NLS declarations
   Copyright (C) 1996, 2007, 2009-2010 Free Software Foundation, Inc.
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

#ifndef _i18n_h_
#define _i18n_h_

/* Take care of NLS matters.  */

#include "gettext.h"

#if ENABLE_NLS
# define _(Text) gettext (Text)
# include <locale.h>
#else
# define _(Text) Text
#endif

#endif /* not _i18n_h_ */
