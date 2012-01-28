# Include this file at the end of each tests/*/Makefile.am.
# Copyright (C) 2007-2012 Free Software Foundation, Inc.

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

built_programs = \
  echo 'spy:;@echo $$(PROGRAMS) $$(SCRIPTS)' \
    | MAKEFLAGS= $(MAKE) -s -C $(top_builddir)/src -f Makefile -f - spy \
    | tr -s ' ' '\n' | sed -e 's,$(EXEEXT)$$,,'

## '$f' is set by the Automake-generated test harness to the path of the
## current test script stripped of VPATH components, and is used by the
## shell-or-perl script to determine the name of the temporary files to be
## used.  Note that $f is a shell variable, not a make macro, so the use of
## '$$f' below is correct, and not a typo.
LOG_COMPILER = \
  $(SHELL) $(srcdir)/shell-or-perl \
  --test-name "$$f" --srcdir '$(srcdir)' \
  --shell '$(SHELL)' --perl '$(PERL)' --

# Note that the first lines are statements.  They ensure that environment
# variables that can perturb tests are unset or set to expected values.
# The rest are envvar settings that propagate build-related Makefile
# variables to test scripts.
TESTS_ENVIRONMENT =				\
  tmp__=$$TMPDIR; test -d "$$tmp__" || tmp__=.;	\
  . $(srcdir)/envvar-check;			\
  TMPDIR=$$tmp__; export TMPDIR;		\
  export					\
  LOCALE_FR='$(LOCALE_FR)'			\
  LOCALE_FR_UTF8='$(LOCALE_FR_UTF8)'		\
  abs_top_builddir='$(abs_top_builddir)'	\
  abs_top_srcdir='$(abs_top_srcdir)'		\
  abs_srcdir='$(abs_srcdir)'			\
  built_programs="`$(built_programs)`"		\
  host_os=$(host_os)				\
  host_triplet='$(host_triplet)'		\
  srcdir='$(srcdir)'				\
  top_srcdir='$(top_srcdir)'			\
  CONFIG_HEADER='$(abs_top_builddir)/lib/config.h' \
  CU_TEST_NAME=`basename '$(abs_srcdir)'`,$$f	\
  AWK='$(AWK)'					\
  EGREP='$(EGREP)'				\
  EXEEXT='$(EXEEXT)'				\
  MAKE=$(MAKE)					\
  PACKAGE_BUGREPORT='$(PACKAGE_BUGREPORT)'	\
  PACKAGE_VERSION=$(PACKAGE_VERSION)		\
  PERL='$(PERL)'				\
  PREFERABLY_POSIX_SHELL='$(PREFERABLY_POSIX_SHELL)' \
  REPLACE_GETCWD=$(REPLACE_GETCWD)		\
  PATH="$(abs_top_builddir)/src$(PATH_SEPARATOR)$(abs_top_srcdir)/src$(PATH_SEPARATOR)$$PATH" \
  VERSION=$(VERSION) \
  ; 9>&2

TEST_LOGS = $(TESTS:=.log)

# Parallel replacement of Automake's check-TESTS target.
SUFFIXES =

VERBOSE = yes
