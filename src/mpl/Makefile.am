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

