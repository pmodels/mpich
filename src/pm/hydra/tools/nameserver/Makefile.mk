## -*- Mode: Makefile; -*-
##
## (C) 2010 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

bin_PROGRAMS += hydra_nameserver

hydra_nameserver_SOURCES = tools/nameserver/hydra_nameserver.c
hydra_nameserver_CFLAGS = $(AM_CFLAGS)
hydra_nameserver_LDFLAGS = $(external_ldflags) 
hydra_nameserver_LDADD = -lhydra $(external_libs)
hydra_nameserver_DEPENDENCIES = libhydra.la

doc1_src_txt += tools/nameserver/hydra_nameserver.txt
