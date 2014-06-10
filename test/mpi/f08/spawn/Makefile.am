# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2014 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS = namepubf90 spawnf90 spawnargvf90 spawnmultf90 spawnmult2f90 connaccf90

# add "-lU77" (or equivalent) to the link line for these tests that use getarg
spawnargvf90_LDADD = $(LDADD) $(FC_GETARG_LIBS)
spawnmultf90_LDADD = $(LDADD) $(FC_GETARG_LIBS)

noinst_PROGRAMS += spawnargvf03 spawnmultf03
spawnargvf03_SOURCES = spawnargvf03.f90
spawnmultf03_SOURCES = spawnmultf03.f90
