/* Special definitions for GNU id-utils, processed by autoheader.
   Copyright (C) 1995, 96 Free Software Foundation, Inc.
*/

#ifndef _config_h_
#define _config_h_

@TOP@

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define if the sbrk system call is declared in unistd.h.  */
#undef HAVE_DECL_SBRK

/* Define if the sys_errlist array is declared in errno.h, error.h or stdio.h.  */
#undef HAVE_DECL_SYS_ERRLIST

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define to 1 if you have the stpcpy function.  */
#undef HAVE_STPCPY

/* Define if you have the <sys/ioctl.h> header file.  */
#undef HAVE_SYS_IOCTL_H

/* Define to the name of the distribution.  */
#undef PACKAGE

/* The concatenation of the strings PACKAGE, "-", and VERSION.  */
#undef PACKAGE_VERSION

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES

/* Define to the major.minor version # of the distribution.  */
#undef VERSION

/* Define to 1 if GNU regex should be used instead of GNU rx.  */
#undef WITH_REGEX

@BOTTOM@

/* According to Thomas Neumann, NeXT POSIX termios support is losing,
   and sgtty is the way to go.  Note: the comment between #undef &
   HAVE_TERMIOS_H is necessary to defeat configure's edits.  */

#if HAVE_SGTTY_H
# ifdef NeXT
#  undef /**/ HAVE_TERMIOS_H
# endif
#endif

#ifndef __P
# ifndef PROTOTYPES
#  define	__P(protos)	()		/* traditional C */
# else
#  define	__P(protos)	protos		/* full-blown ANSI C */
# endif
#endif

#endif /* not _config_h_ */
