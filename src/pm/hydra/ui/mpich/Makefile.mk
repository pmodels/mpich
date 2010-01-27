# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/ui/utils

bin_PROGRAMS += mpiexec

mpiexec_SOURCES = $(top_srcdir)/ui/mpich/mpiexec.c $(top_srcdir)/ui/mpich/utils.c
mpiexec_LDFLAGS = $(external_ldflags) -L$(top_builddir)
mpiexec_LDADD = -lpm -lhydra $(external_libs)
mpiexec_DEPENDENCIES = libpm.la libhydra.la
