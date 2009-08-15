## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libidu.a

libidu_a_SOURCES = dynvec.c dynvec.h \
                   idu-hash.c idu-hash.h \
                   idfile.c idfile.h \
                   idread.c  \
                   idwrite.c \
                   fnprint.c \
                   scanners.c scanners.h \
                   walker.c \
                   tokflags.h \
                   iduglobal.h \
                   xnls.h

AM_CPPFLAGS = -I$(top_srcdir)/lib \
              -DDATADIR=\"$(datadir)\" \
              -DLOCALEDIR=\"$(datadir)/locale\" \
              -DLANGUAGE_MAP_FILE=\"$(datadir)/id-lang.map\"

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

idudir = $(datadir)
idu_DATA = id-lang.map
EXTRA_DIST = $(idu_DATA)
