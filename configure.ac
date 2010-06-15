#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([idutils], m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[bug-idutils@gnu.org])

AC_CONFIG_AUX_DIR(build-aux)
AM_INIT_AUTOMAKE([1.11 dist-xz color-tests parallel-tests std-options])
AM_SILENT_RULES([yes]) # make --enable-silent-rules the default.

AC_CONFIG_SRCDIR([src/mkid.c])
AC_CONFIG_HEADERS([lib/config.h:lib/config.hin])

# Checks for programs.
AM_MISSING_PROG(HELP2MAN, help2man)
AC_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_LN_S
gl_EARLY
gl_INIT

AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings],
		  [turn on lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])
  AC_SUBST([WERROR_CFLAGS])

  # Add many warnings, except some...
  nw="$nw -Wdeclaration-after-statement" # too useful to forbid
  nw="$nw -Waggregate-return"       # anachronistic
  nw="$nw -Wlong-long"              # C90 is anachronistic (lib/gethrxtime.h)
  nw="$nw -Wc++-compat"             # We don't care about C++ compilers
  nw="$nw -Wundef"                  # Warns on '#if GNULIB_FOO' etc in gnulib
  nw="$nw -Wtraditional"            # Warns on #elif which we use often
  nw="$nw -Wcast-qual"              # Too many warnings for now
  nw="$nw -Wconversion"             # Too many warnings for now
  nw="$nw -Wsystem-headers"         # Don't let system headers trigger warnings
  nw="$nw -Wsign-conversion"        # Too many warnings for now
  nw="$nw -Wtraditional-conversion" # Too many warnings for now
  nw="$nw -Wunreachable-code"       # Too many warnings for now
  nw="$nw -Wpadded"                 # Our structs are not padded
  nw="$nw -Wredundant-decls"        # openat.h declares e.g., mkdirat
  nw="$nw -Wlogical-op"             # any use of fwrite provokes this
  nw="$nw -Wformat-nonliteral"      # who.c and pinky.c strftime uses
  nw="$nw -Wvla"                    # warnings in gettext.h
  nw="$nw -Wnested-externs"         # use of XARGMATCH/verify_function__
  nw="$nw -Wswitch-enum"            # Too many warnings for now
  nw="$nw -Wswitch-default"         # Too many warnings for now
  # things I might fix soon:
  nw="$nw -Wstrict-overflow"        # fid.c
  nw="$nw -Wunsafe-loop-optimizations" # mkid.c

  gl_MANYWARN_ALL_GCC([ws])
  gl_MANYWARN_COMPLEMENT([ws], [$ws], [$nw])
  for w in $ws; do
    gl_WARN_ADD([$w])
  done
  gl_WARN_ADD([-Wno-missing-field-initializers]) # We need this one
  gl_WARN_ADD([-Wno-sign-compare])     # Too many warnings for now
  gl_WARN_ADD([-Wno-pointer-sign])     # Too many warnings for now
  gl_WARN_ADD([-Wno-unused-parameter]) # Too many warnings for now
  gl_WARN_ADD([-fdiagnostics-show-option])

  AC_SUBST([WARN_CFLAGS])

  AC_DEFINE([lint], [1], [Define to 1 if the compiler is checking for lint.])
  AC_DEFINE([_FORTIFY_SOURCE], [2],
    [enable compile-time and run-time bounds-checking, and some warnings])
  AC_DEFINE([GNULIB_PORTCHECK], [1], [enable some gnulib portability checks])
fi

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

AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION([0.18])

AC_CONFIG_FILES([
                 po/Makefile.in
                 doc/Makefile
                 man/Makefile
                 lib/Makefile
                 libidu/Makefile
                 lisp/Makefile
                 testsuite/Makefile
                 gnulib-tests/Makefile
                 src/Makefile
		 Makefile
                ])
AC_OUTPUT
