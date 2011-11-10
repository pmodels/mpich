# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

ACLOCAL_AMFLAGS = -I confdb
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libmpl.la
libmpl_la_SOURCES = src/mplstr.c src/mpltrmem.c src/mplenv.c
libmpl_la_LDFLAGS = -version-info ${libmpl_so_version}

MPL_TESTS = strsep
TESTS = $(MPL_TESTS)
check_PROGRAMS = $(MPL_TESTS)
strsep_SOURCES = test/strsep.c
strsep_LDADD = libmpl.la

mpl_headers =              \
    include/mpl.h          \
    include/mpl_utlist.h   \
    include/mpl_valgrind.h \
    include/mplconfig.h    \
    include/mplenv.h       \
    include/mplstr.h       \
    include/mpltrmem.h

if MPL_EMBEDDED_MODE
noinst_HEADERS = $(mpl_headers)
else !MPL_EMBEDDED_MODE
include_HEADERS = $(mpl_headers)
endif !MPL_EMBEDDED_MODE

# files that automake doesn't know about by default that should be included in
# the distribution
EXTRA_DIST = VERSION

DISTCLEANFILES = _configs.sed include/mplconfig.h
