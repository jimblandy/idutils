# Customize maint.mk.                           -*- makefile -*-
# Copyright (C) 2003-2010 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Used in maint.mk's web-manual rule
manual_title = GNU idutils: ID database utilities

# Tests not to run as part of "make distcheck".
# Exclude changelog-check here so that there's less churn in ChangeLog
# files -- otherwise, you'd need to have the upcoming version number
# at the top of the file for each `make distcheck' run.
local-checks-to-skip = changelog-check strftime-check patch-check \
  sc_prohibit_atoi_atof sc_changelog check-AUTHORS

old_NEWS_hash = f4306613b611173d9c781781acec9aa9

sc_tight_scope:
	$(MAKE) -C src $@

include $(srcdir)/dist-check.mk

update-copyright-env = \
  UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  UPDATE_COPYRIGHT_MAX_LINE_LENGTH=79

_hv_file = $(srcdir)/testsuite/help-version
