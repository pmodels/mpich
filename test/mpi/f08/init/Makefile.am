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

noinst_PROGRAMS = baseenvf90

EXTRA_PROGRAMS = checksizes
# Checksizes is created by configure, and is a C program that tests that
# configure has correctly computed the sizes of MPI_Aint and MPI_Offset
# for Fortran.  This is not run by default but can be used to check
# that configure found the correct values.
checksizes_LDADD =
nodist_checksizes_SOURCES = checksizes.c

DISTCLEANFILES = checksizes.c
