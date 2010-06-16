## Process this file with automake to create Makefile.in

TESTS =			\
  consistency		\
  files0-from		\
  help-version		\
  infloop-kawa-el

EXTRA_DIST =			\
  $(TESTS)			\
  Coreutils.pm			\
  CuTmpdir.pm			\
  check.mk			\
  envvar-check			\
  init.sh			\
  mkid-langopt			\
  single_file_token_bug.c

DISTCLEANFILES = ID

AUTOMAKE_OPTIONS =
include $(srcdir)/check.mk
