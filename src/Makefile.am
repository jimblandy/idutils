## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS = ansi2knr

bin_PROGRAMS = mkid lid fid idx
bin_SCRIPTS = eid aid gid pid defid

EXTRA_DIST = ansi2knr.1 ansi2knr.c $(bin_SCRIPTS)

localedir = $(datadir)/locale

INCLUDES = -I. -I$(srcdir) \
	   -I../lib -I$(top_srcdir)/lib \
	   -I../intl -I$(top_srcdir)/intl \
	   -I.. -I$(top_srcdir) 
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LDADD = @INTLLIBS@ ../lib/libidu.a

$(PROGRAMS): $(LDADD)
