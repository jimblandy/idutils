## Process this file with automake to create Makefile.in

TESTS = consistency

EXTRA_DIST = $(TESTS)

consistency-real: all
	$(SHELL) $(srcdir)/consistency $(top_srcdir) $(top_builddir)/src
