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

# When the version number is composed of only digits and "."s,
# ensure that it matches the "Version:" comment in idutils.el.
f = $(srcdir)/lisp/idutils.el
distcheck-hook:
	case "$(VERSION)" in *[^.0-9]*) chk=n;; *) chk=y;; esac;	\
	if test $$chk = y; then						\
	  v=`echo "$(VERSION)" | sed 's/\./\\\\./'`;			\
	  grep '^;; Version: '"$$v"'$$' $(f) > /dev/null && :		\
	    || { echo 1>&2 $(f): update version to $(VERSION);exit 1; }	\
	else								\
	  :;								\
	fi

dist-hook:
	if test -d .git; then						\
	  $(top_srcdir)/build-aux/gitlog-to-changelog --since=2008-01-01 \
	    > $(distdir)/cl-t;						\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi
