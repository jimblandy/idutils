## Process this file with automake to create Makefile.in

EXTRA_DIST = id-utils.el elisp-comp
elc_SCRIPTS = id-utils.elc
elcdir = @LISPDIR@
SUFFIXES = .el .elc

.el.elc:
	@echo "WARNING: Warnings can be ignored. :-)"
	$(SHELL) $(srcdir)/elisp-comp $<
