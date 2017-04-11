## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/mpiexec

noinst_HEADERS += \
	mpiexec/mpiexec.h

bin_PROGRAMS += mpiexec.hydra

mpiexec_hydra_SOURCES = \
	mpiexec/mpiexec.c \
	mpiexec/mpiexec_params.c \
	mpiexec/mpiexec_pmi.c \
	mpiexec/mpiexec_utils.c

mpiexec_hydra_LDFLAGS = $(external_ldflags) -L$(top_builddir)
mpiexec_hydra_LDADD = -lmpx -lhydra $(external_libs)
mpiexec_hydra_DEPENDENCIES = libmpx.la libhydra.la @mpl_lib@
