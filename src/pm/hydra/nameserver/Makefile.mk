##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

bin_PROGRAMS += hydra_nameserver

hydra_nameserver_SOURCES = nameserver/hydra_nameserver.c
hydra_nameserver_CFLAGS = $(AM_CFLAGS)
hydra_nameserver_LDFLAGS = $(external_ldflags) 
hydra_nameserver_LDADD = $(internal_libs) $(external_libs)
hydra_nameserver_DEPENDENCIES = $(internal_libs)
