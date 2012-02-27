# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f" lines
AM_DEFAULT_SOURCE_EXT = .f

noinst_PROGRAMS = baseenvf

EXTRA_PROGRAMS = checksizes
# Checksizes is created by configure, and is a C program that tests that 
# configure has correctly computed the sizes of MPI_Aint and MPI_Offset
# for Fortran.  This is not run by default but can be used to check
# that configure found the correct values.
checksizes_LDADD = 
nodist_checksizes_SOURCES = checksizes.c

DISTCLEANFILES = checksizes.c

