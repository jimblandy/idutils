## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = m4 intl lib libidu src lisp doc testsuite po
EXTRA_DIST = README-alpha config.rpath mkinstalldirs autogen.sh

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
