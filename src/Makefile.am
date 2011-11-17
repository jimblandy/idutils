bin_PROGRAMS = mkid lid fid fnid xtokid aid eid gid
dist_bin_SCRIPTS = defid

noinst_HEADERS = lid.h
aid_SOURCES = lid.c lid-aid.c
eid_SOURCES = lid.c lid-eid.c
gid_SOURCES = lid.c lid-gid.c
lid_SOURCES = lid.c lid-lid.c

AM_CPPFLAGS = -I$(top_srcdir)/lib \
              -I$(top_srcdir)/libidu \
              -DDATADIR=\"$(datadir)\" \
              -DLOCALEDIR=\"$(datadir)/locale\" \
              -DLANGUAGE_MAP_FILE=\"$(datadir)/id-lang.map\"

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

LDADD = ../libidu/libidu.a ../lib/libgnu.a $(LIBINTL) ../lib/libgnu.a

# Tell automake's installcheck-binPROGRAMS rule that defid
# need not be checked for --help and --version support.
# Otherwise, it would usually fail when gid is not already installed.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = defid
