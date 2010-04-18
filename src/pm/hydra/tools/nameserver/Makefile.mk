# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += hydra_nameserver

hydra_nameserver_SOURCES = $(top_srcdir)/tools/nameserver/hydra_nameserver.c
hydra_nameserver_CFLAGS = $(AM_CFLAGS)
hydra_nameserver_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_nameserver_LDADD = -lhydra $(external_libs)
hydra_nameserver_DEPENDENCIES = libhydra.la
