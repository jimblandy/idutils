/* Special definitions for `mkid', processed by autoheader.
   This file is in the public domain.
*/

/* Define if you have the <sys/ioctl.h> header file.  */
#undef HAVE_SYS_IOCTL_H

/* Define to the name of the distribution.  */
#undef PRODUCT

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES

/* Define to the major version # of the distribution.  */
#undef MAJOR_VERSION

/* Define to the minor version # of the distribution.  */
#undef MINOR_VERSION

/* Define to the minor.minor version # of the distribution.  */
#undef VERSION

/* Define to the minor.minor.patch_level # of the distribution.  */
#undef FULL_VERSION

/* Define to the patch level of the distribution.  */
#undef PATCH_LEVEL

/* Define to filename of iid help text.  */
#undef IID_HELP_FILE

@BOTTOM@

#if SIZEOF_LONG == 4
typedef unsigned long uint32_t;
typedef long int32_t;
#else /* SIZEOF_LONG != 4 */
#if SIZEOF_INT == 4
typedef unsigned int uint32_t;
typedef int int32_t;
#else /* SIZEOF_INT != 4 */
#error "Your system is weird.  What integer has sizeof () == 4 ???"
#endif /* SIZEOF_INT != 4 */
#endif /* SIZEOF_LONG != 4 */

#if SIZEOF_SHORT == 2
typedef unsigned short uint16_t;
#else
#error "Your system is weird.  sizeof (short) != 2"
#endif

#if SIZEOF_CHAR == 1
typedef unsigned char uint8_t;
#else
#error "Your system is weird.  sizeof (char) != 1"
#endif
