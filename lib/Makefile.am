## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS = ansi2knr

noinst_LIBRARIES = idu

EXTRA_DIST = alloca.c error.c getopt.c getopt1.c regex.c rx.c \
	strcasecmp.c strdup.c strndup.c strtok.c \
	obstack.c basename.c dirname.c fnmatch.c \
	ansi2knr.1 ansi2knr.c 

idu_SOURCES = misc.c scanners.c idfile.c filenames.c bitops.c token.c \
	idwalk.c hash.c dynvec.c xmalloc.c xgetcwd.c

idudir = $(prefix)/share
idu_DATA = language.map

noinst_HEADERS = alloc.h bitops.h error.h filenames.h getopt.h idarg.h \
	idfile.h misc.h regex.h rx.h scanners.h strxtra.h \
	token.h system.h hash.h obstack.h pathmax.h dynvec.h \
	xmalloc.h fnmatch.h ansidecl.h

INCLUDES = -I. -I$(srcdir) \
	   -I../lib -I$(top_srcdir)/lib \
	   -I../intl -I$(top_srcdir)/intl \
	   -I.. -I$(top_srcdir) 
DEFS = -DLOCALEDIR=\"$(localedir)\" -DLANGUAGE_MAP=\"$(idudir)/language.map\" @DEFS@
idu_LIBADD = @REGEXOBJ@ @LIBOBJS@ @ALLOCA@
