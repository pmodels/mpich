##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

bin_PROGRAMS += hydra_persist

hydra_persist_SOURCES = persist_server/persist_server.c
hydra_persist_CFLAGS = $(AM_CFLAGS)
hydra_persist_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_persist_LDADD = $(internal_libs) $(external_libs)
hydra_persist_DEPENDENCIES = $(internal_libs)
