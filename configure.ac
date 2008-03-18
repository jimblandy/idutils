#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([idutils], m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[bug-idutils@gnu.org])

AC_CONFIG_AUX_DIR(build-aux)
AM_INIT_AUTOMAKE([1.10])

AC_CONFIG_SRCDIR([src/mkid.c])
AC_CONFIG_HEADERS([lib/config.h:lib/config.hin])

# Checks for programs.
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_LN_S
gl_EARLY
gl_INIT

AC_PROG_INSTALL
AC_PROG_RANLIB

# check functions

# if HAVE_LINK, then in the code we look for file aliases
# if HAVE_SBRK, then we can generate statistics on memory usage

AC_CHECK_FUNCS([link sbrk lstat])

AM_PATH_LISPDIR

# Checks for header files.

AC_CHECK_HEADERS([termios.h sys/ioctl.h termio.h sgtty.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ptrdiff_t)

AM_GNU_GETTEXT([external], [need-formatstring-macros])
AM_GNU_GETTEXT_VERSION([0.15])

dnl Allow maintainer rules under GNU make even in VPATH builds.  This does
dnl not work in autoconf 2.61 or earlier, but we don't want to require
dnl non-maintainers to use unreleased autoconf, hence the version test.
dnl TODO remove the version check once autoconf 2.62 is released.
dnl Meanwhile, we must use a shell variable so that we bypass automake's
dnl attempts to remove the sole copy of GNUmakefile in a non-VPATH build
dnl during 'make distclean'.
dnl TODO avoid the shell variable once automake is fixed.
GNUmakefile=GNUmakefile
m4_if(m4_version_compare([2.61a.100],
	m4_defn([m4_PACKAGE_VERSION])), [1], [],
      [AC_CONFIG_LINKS([$GNUmakefile:$GNUmakefile], [],
	[GNUmakefile=$GNUmakefile])])

AC_CONFIG_FILES([
                 po/Makefile.in
                 doc/Makefile
                 lib/Makefile
                 libidu/Makefile
                 lisp/Makefile
                 testsuite/Makefile
                 gnulib-tests/Makefile
                 src/Makefile
		 Makefile
                ])
AC_OUTPUT
