# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/ui/utils -DHYDRA_CONF_FILE=\"@sysconfdir@/mpiexec.hydra.conf\"

bin_PROGRAMS += mpiexec.hydra

mpiexec_hydra_SOURCES = $(top_srcdir)/ui/mpich/mpiexec.c $(top_srcdir)/ui/mpich/utils.c
mpiexec_hydra_LDFLAGS = $(external_ldflags) -L$(top_builddir)
mpiexec_hydra_LDADD = -lpm -lhydra $(external_libs)
mpiexec_hydra_DEPENDENCIES = libpm.la libhydra.la
