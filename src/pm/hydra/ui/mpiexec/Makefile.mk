# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += mpiexec

mpiexec_SOURCES = $(top_srcdir)/ui/mpiexec/callback.c \
	$(top_srcdir)/ui/mpiexec/mpiexec.c \
	$(top_srcdir)/ui/mpiexec/utils.c
mpiexec_LDADD = libui.a libpm.a libhydra.a $(external_libs)
mpiexec_CFLAGS = -I$(top_srcdir)/ui/utils
mpiexec_LDFLAGS = $(external_ldflags)
