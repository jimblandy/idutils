## Process this file with automake to produce Makefile.in

SUBDIRS = lib intl src doc po

TESTS = idtest

EXTRA_DIST = idtest @README_ALPHA@

idtest-real: all
	$(SHELL) $(top_srcdir)/idtest $(top_srcdir) $(top_builddir)
