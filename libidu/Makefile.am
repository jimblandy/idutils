## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libidu.a

libidu_a_SOURCES = dynvec.c dynvec.h \
                   hash.c hash.h \
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
              -I$(top_builddir)/intl \
              -DDATADIR=\"$(datadir)\" \
              -DLOCALEDIR=\"$(datadir)/locale\" \
              -DLANGUAGE_MAP_FILE=\"$(datadir)/id-lang.map\"

idudir = $(datadir)
idu_DATA = id-lang.map
EXTRA_DIST = $(idu_DATA)
