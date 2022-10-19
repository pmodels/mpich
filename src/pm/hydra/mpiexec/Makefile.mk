##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_PROGRAMS += mpiexec.hydra

mpiexec_hydra_SOURCES = \
    mpiexec/mpiexec.c \
    mpiexec/options.c \
    mpiexec/get_parameters.c \
    mpiexec/uiu.c \
    mpiexec/pg.c \
    mpiexec/debugger.c \
    mpiexec/pmiserv_kvs.c \
    mpiexec/pmiserv_publish.c \
    mpiexec/pmiserv_spawn.c \
    mpiexec/pmiserv_misc.c \
    mpiexec/pmiserv_cb.c \
    mpiexec/pmiserv_pmi.c \
    mpiexec/pmiserv_pmci.c \
    mpiexec/pmiserv_utils.c

mpiexec_hydra_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/mpiexec -DHYDRA_CONF_FILE=\"@sysconfdir@/mpiexec.hydra.conf\"
mpiexec_hydra_LDFLAGS = $(external_ldflags) -L$(top_builddir)
mpiexec_hydra_LDADD = -lhydra $(external_libs)
mpiexec_hydra_DEPENDENCIES = libhydra.la
