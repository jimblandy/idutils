## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib libidu src lisp doc man gnulib-tests testsuite po
EXTRA_DIST = \
  .prev-version \
  .version \
  .x-sc_cast_of_argument_to_free \
  .x-sc_require_config_h \
  cfg.mk \
  dist-check.mk \
  idutils.spec \
  idutils.spec.in \
  maint.mk \
  bootstrap \
  bootstrap.conf \
  build-aux/vc-list-files

gen_start_date = 2008-01-01
.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d .git; then						\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    --since=$(gen_start_date) > $(distdir)/cl-t;		\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi

idutils.spec: idutils.spec.in .version
	sed 's/@''PACKAGE@/$(PACKAGE)/;s/@''VERSION@/$(VERSION)/' $< > $@-t
	mv $@-t $@
MAINTAINERCLEANFILES = idutils.spec

BUILT_SOURCES = .version
.version:
	echo $(VERSION) > $@-t && mv $@-t $@

# Arrange so that .tarball-version appears only in the distribution
# tarball, and never in a checked-out repository.
# The perl substitution is to change some key uses of "rm" to "/bin/rm".
# See the rm_subst comment for details.
dist-hook: gen-ChangeLog
	echo $(VERSION) > $(distdir)/.tarball-version
