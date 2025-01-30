##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = autogen.sh README.md
EXTRA_PROGRAMS =

.PHONY: doxygen

pkgconfigdir = $(libdir)/pkgconfig
if EMBEDDED_BUILD
pkgconfig_DATA =
else
pkgconfig_DATA = maint/yaksa.pc
endif !EMBEDDED_BUILD

noinst_HEADERS =
include_HEADERS =
noinst_PROGRAMS =
nodist_noinst_SCRIPTS =
dist_noinst_SCRIPTS = autogen.sh

if EMBEDDED_BUILD
noinst_LTLIBRARIES = libyaksa.la
else
lib_LTLIBRARIES = libyaksa.la
endif !EMBEDDED_BUILD

libyaksa_la_SOURCES =
AM_CPPFLAGS =

if EMBEDDED_BUILD
libyaksa_la_LDFLAGS = -avoid-version
else
libyaksa_la_LDFLAGS = -version-info @libyaksa_so_version@
endif !EMBEDDED_BUILD

include $(top_srcdir)/src/Makefile.mk

testlists=
include $(top_srcdir)/test/Makefile.mk
include $(top_srcdir)/examples/Makefile.mk

examples: $(example_list)

doxygen:
	mkdir -p doc
	doxygen Doxyfile

testing:
	@$(top_srcdir)/test/runtests.py --summary=$(top_builddir)/test/summary.junit.xml \
                $(testlists)
