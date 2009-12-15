# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/ui/utils

bin_PROGRAMS += mpiexec

mpiexec_SOURCES = $(top_srcdir)/ui/mpich/mpiexec.c \
	$(top_srcdir)/ui/mpich/utils.c
mpiexec_LDADD = libui.a libpm.a libhydra.a $(external_libs)
mpiexec_LDFLAGS = $(external_ldflags)
