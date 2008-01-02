## Process this file with automake to create Makefile.in

TESTS = \
  consistency \
  infloop-kawa-el

TESTS_ENVIRONMENT = \
  abs_top_srcdir='$(abs_top_srcdir)'		\
  PATH="$(abs_builddir)/../src$(PATH_SEPARATOR)$$PATH"

EXTRA_DIST = $(TESTS) single_file_token_bug.c

DISTCLEANFILES = ID

consistency-real: all
	$(SHELL) $(srcdir)/consistency $(top_srcdir) $(top_builddir)/src
