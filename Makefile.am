## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib libidu src lisp doc testsuite po gnulib-tests
EXTRA_DIST = \
  .prev-version \
  .x-sc_cast_of_argument_to_free \
  .x-sc_require_config_h \
  GNUmakefile \
  Makefile.cfg \
  Makefile.maint \
  bootstrap \
  bootstrap.conf \
  build-aux/vc-list-files

dist-hook:
	if test -d .git; then						\
	  $(top_srcdir)/build-aux/gitlog-to-changelog --since=2008-01-01 \
	    > $(distdir)/cl-t;						\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi
