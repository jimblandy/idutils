#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([id-utils], [4.1rc1], [bug-gnu-utils@gnu.org])
AC_GNU_SOURCE

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
gl_EARLY

AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ALLOCA

# gettext support

AM_GNU_GETTEXT

# Use HAVE_LINK as discriminator between Unix systems and others
AC_CHECK_FUNCS(link,
  [AC_MSG_NOTICE([assuming Unix conventions])],
  [AC_MSG_NOTICE([assuming non-Unix])])

AM_PATH_LISPDIR

# gnulib

gl_INIT

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES(ptrdiff_t)

AC_CONFIG_FILES([
                 intl/Makefile
                 po/Makefile.in
                 doc/Makefile
                 m4/Makefile
                 lib/Makefile
                 libidu/Makefile
                 lisp/Makefile
                 testsuite/Makefile
                 src/Makefile
		 Makefile
                ])
AC_OUTPUT




