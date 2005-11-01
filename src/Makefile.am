
bin_PROGRAMS = mkid lid fid fnid xtokid
dist_bin_SCRIPTS = eid aid gid defid

AM_CPPFLAGS = -I$(top_srcdir)/libidu \
              -I$(top_srcdir)/lib \
              -I$(top_srcdir)/intl \
              -DDATADIR=\"$(datadir)\" \
              -DLOCALEDIR=\"$(datadir)/locale\" \
              -DLANGUAGE_MAP_FILE=\"$(datadir)/id-lang.map\"

LDADD = ../libidu/libidu.a $(LIBINTL) ../lib/libgnu.a 

