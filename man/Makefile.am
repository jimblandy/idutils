## Process this file with automake to produce Makefile.in -*-Makefile-*-
dist_man1_MANS = mkid.1 lid.1 fid.1 fnid.1 xtokid.1 eid.1 aid.1 gid.1 defid.1

man_aux = $(dist_man1_MANS:.1=.x)
EXTRA_DIST = $(man_aux)
MAINTAINERCLEANFILES = $(dist_man1_MANS)

# Depend on .version to get version number changes.
common_dep = ../.version

mkid.1:		$(common_dep)	$(srcdir)/mkid.x	../src/mkid.c
lid.1:		$(common_dep)	$(srcdir)/lid.x		../src/lid.c
fid.1:		$(common_dep)	$(srcdir)/fid.x		../src/fid.c
fnid.1:		$(common_dep)	$(srcdir)/fnid.x	../src/fnid.c
xtokid.1:	$(common_dep)	$(srcdir)/xtokid.x	../src/xtokid.c
aid.1:		$(common_dep)	$(srcdir)/aid.x		../src/lid.c
eid.1:		$(common_dep)	$(srcdir)/eid.x		../src/lid.c
gid.1:		$(common_dep)	$(srcdir)/gid.x		../src/lid.c
defid.1:	$(common_dep)	$(srcdir)/defid.x	../src/lid.c

SUFFIXES = .x .1
.x.1:
	$(AM_V_GEN)PATH=../src$(PATH_SEPARATOR)$$PATH; export PATH;	\
	$(HELP2MAN)							\
	    --include=$(srcdir)/$*.x					\
	    --output=$@ $*$(EXEEXT)

check-local: check-x-vs-1 check-programs-vs-x

# Sort in traditional ASCII order, regardless of the current locale;
# otherwise we may get into trouble with distinct strings that the
# current locale considers to be equal.
ASSORT = LC_ALL=C sort

# Ensure that for each .x file in this directory, there is a
# corresponding .1 file in the definition of $(dist_man1_MANS) above.
.PHONY: check-x-vs-1
check-x-vs-1:
	$(AM_V_GEN)PATH=../src$(PATH_SEPARATOR)$$PATH; export PATH;	\
	t=ls-files.$$$$;						\
	(cd $(srcdir) && ls -1 *.x) | sed 's/\.x$$//' | $(ASSORT) > $$t;\
	echo $(dist_man1_MANS) | tr -s ' ' '\012' | sed 's/\.1$$//'	\
	  | $(ASSORT) | diff - $$t || { rm $$t; exit 1; };		\
	rm $$t

programs =								\
  echo 'spy:;@echo $$(PROGRAMS) $$(SCRIPTS)'				\
    | MAKEFLAGS= $(MAKE) -s -C $(srcdir)/../src -f Makefile -f - spy	\
    | tr -s ' ' '\012' | $(ASSORT) -u

.PHONY: check-programs-vs-x
check-programs-vs-x:
	$(AM_V_GEN)for p in `$(programs)`; do		\
	  test -f $(srcdir)/$$p.x			\
	    || { echo missing $$p.x 1>&2; missing=1; };	\
	done;						\
	test "$$missing" = 1 && exit 1 || :
