##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

bin_PROGRAMS += hydra_nameserver

hydra_nameserver_SOURCES = nameserver/hydra_nameserver.c
hydra_nameserver_CFLAGS = $(AM_CFLAGS)
hydra_nameserver_LDFLAGS = $(external_ldflags) 
hydra_nameserver_LDADD = -lhydra $(external_libs)
hydra_nameserver_DEPENDENCIES = libhydra.la
