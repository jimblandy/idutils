/* Special definitions for `mkid', processed by autoheader.
   This file is in the public domain.
*/

#ifndef _config_h_
#define _config_h_

@TOP@

/* Define to `unsigned short' if <sys/types.h> doesn't define.  */
#undef dev_t

/* Define if you have the <sys/ioctl.h> header file.  */
#undef HAVE_SYS_IOCTL_H

/* Define to filename of iid help text.  */
#undef IID_HELP_FILE

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
#undef ino_t

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES

/* Define to the major.minor version # of the distribution.  */
#undef VERSION

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
#  define __P(args) ()
# else
#  define __P(args) args
# endif
#endif

#endif /* not _config_h_ */
