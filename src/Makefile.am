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

# Most functions in src/*.c should have static scope.
# Any that don't must be marked with `extern', but `main'
# and `usage' are exceptions.  They're always extern, but
# don't need to be marked.
#
# The second nm|grep checks for file-scope variables with `extern' scope.
.PHONY: sc_tight_scope
sc_tight_scope: $(all_programs)
	@t=exceptions-$$$$;						\
	trap "s=$$?; rm -f $$t; exit $$s" 0 1 2 13 15;			\
	( printf '^main$$\n^usage$$\n';					\
	  perl -ne '/^extern .*?\**(\w+) *\(/ and print "^$$1\$$\n"'	\
	      $(noinst_HEADERS) $(srcdir)/../libidu/*.h ) > $$t;	\
	nm -e *.$(OBJEXT) ../libidu/*.$(OBJEXT)				\
	    | sed -n 's/.* T //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above functions should have static scope' 1>&2;	\
	    exit 1; } || : ;						\
	( printf '^program_name$$\n';					\
	  perl -ne '/^extern .*?\**(\w+);/ and print "^$$1\$$\n"'	\
	      $(noinst_HEADERS) $(srcdir)/../libidu/*.h ) > $$t;	\
	nm -e *.$(OBJEXT) ../libidu/*.$(OBJEXT)				\
	    | sed -n 's/.* [BD] //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above variables should have static scope' 1>&2;	\
	    exit 1; } || :
