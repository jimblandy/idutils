/* Special definitions for `mkid', processed by autoheader.
   This file is in the public domain.
*/

#ifndef _config_h_
#define _config_h_

@TOP@

/* Define if you have the <sys/ioctl.h> header file.  */
#undef HAVE_SYS_IOCTL_H

/* Define to filename of iid help text.  */
#undef IID_HELP_FILE

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to the patch-level of the distribution.  */
#undef PATCH_LEVEL

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES

/* Define to the major.minor version # of the distribution.  */
#undef VERSION

@BOTTOM@

/* (u)intmin32_t are integer types that are *at least* 32 bits.
   Larger ints are OK.  */
# if SIZEOF_LONG == 4
  typedef unsigned long uintmin32_t;
  typedef long intmin32_t;
# else
  typedef unsigned int uintmin32_t;
  typedef int intmin32_t;
# endif

#endif /* _config_h_ */
