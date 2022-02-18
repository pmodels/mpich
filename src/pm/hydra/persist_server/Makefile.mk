##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_PROGRAMS += hydra_persist

hydra_persist_SOURCES = persist_server/persist_server.c
hydra_persist_CFLAGS = $(AM_CFLAGS)
hydra_persist_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_persist_LDADD = -lhydra $(external_libs)
hydra_persist_DEPENDENCIES = libhydra.la
