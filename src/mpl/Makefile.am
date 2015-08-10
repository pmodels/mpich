# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include

noinst_LTLIBRARIES = lib@MPLLIBNAME@.la
lib@MPLLIBNAME@_la_SOURCES = \
	src/mplstr.c \
	src/mpltrmem.c \
	src/mplenv.c \
	src/mplmsg.c \
	src/mplsock.c
lib@MPLLIBNAME@_la_LDFLAGS = ${lib@MPLLIBNAME@_so_versionflags}

MPL_TESTS = strsep
TESTS = $(MPL_TESTS)
check_PROGRAMS = $(MPL_TESTS)
strsep_SOURCES = test/strsep.c
strsep_LDADD = lib@MPLLIBNAME@.la

mpl_headers =              \
    include/mpl.h          \
    include/mpl_utlist.h   \
    include/mpl_valgrind.h \
    include/mplconfig.h    \
    include/mplenv.h       \
    include/mplstr.h       \
    include/mpltrmem.h     \
    include/mplmsg.h       \
    include/mpliov.h       \
    include/mplsock.h

if MPL_EMBEDDED_MODE
noinst_HEADERS = $(mpl_headers)
else !MPL_EMBEDDED_MODE
include_HEADERS = $(mpl_headers)
endif !MPL_EMBEDDED_MODE

# --------------------------------------------------------------------------
.PHONY: coverage
gcov_sources = $(libmpl_la_SOURCES)
# assumes that these sources were compiled appropriately ("-fprofile-arcs"
# and "-ftest-coverage")
coverage:
	@for file in $(gcov_sources) ; do \
	    dir=`dirname $$file` ; \
	    bname=`basename $$file` ; \
	    aux=`echo $$bname | sed -e 's,\.*$$,,'` ; \
	    echo "( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir )" ; \
	    ( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir ) ; \
	    rm -f *.gcov ; \
	done
	for subdir in $(SUBDIRS) - ; do \
	    if test $$subdir = "-" ; then break ; fi ; \
	    ( cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) coverage ) ; \
	done
# --------------------------------------------------------------------------

# files that automake doesn't know about by default that should be included in
# the distribution
EXTRA_DIST = VERSION

DISTCLEANFILES = _configs.sed include/mplconfig.h
