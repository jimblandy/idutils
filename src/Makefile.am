
bin_PROGRAMS = mkid lid fid fnid xtokid
dist_bin_SCRIPTS = eid aid gid defid

AM_CPPFLAGS = -I$(top_builddir)/intl \
              -I$(top_srcdir)/lib \
              -I$(top_srcdir)/libidu \
              -DDATADIR=\"$(datadir)\" \
              -DLOCALEDIR=\"$(datadir)/locale\" \
              -DLANGUAGE_MAP_FILE=\"$(datadir)/id-lang.map\"

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
	  grep -h -A1 '^extern .*[^;]$$' $(SOURCES)			\
	    | grep -vE '^(extern |--)' |sed 's/^/^/;s/ .*/$$/' ) > $$t;	\
	nm -e *.$(OBJEXT) ../libidu/*.$(OBJEXT)				\
	    | sed -n 's/.* T //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above functions should have static scope' 1>&2;	\
	    exit 1; } || : ;						\
	( printf '^program_name$$\n';					\
	  sed -n 's/^extern int \([^ ][^ ]*\);$$/^\1$$/p'		\
	    $(noinst_HEADERS) /dev/null ) > $$t;			\
	nm -e *.$(OBJEXT) ../libidu/*.$(OBJEXT)				\
	    | sed -n 's/.* [BD] //p'					\
	    | grep -Ev -f $$t &&					\
	  { echo 'the above variables should have static scope' 1>&2;	\
	    exit 1; } || :
