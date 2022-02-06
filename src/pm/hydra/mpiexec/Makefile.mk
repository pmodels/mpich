##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_PROGRAMS += mpiexec.hydra

mpiexec_hydra_SOURCES = \
    mpiexec/mpiexec.c \
    mpiexec/utils.c \
    mpiexec/uiu.c

mpiexec_hydra_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/mpiexec -DHYDRA_CONF_FILE=\"@sysconfdir@/mpiexec.hydra.conf\"
mpiexec_hydra_LDFLAGS = $(external_ldflags) -L$(top_builddir)
mpiexec_hydra_LDADD = -lpm -lhydra $(external_libs)
mpiexec_hydra_DEPENDENCIES = libpm.la libhydra.la
