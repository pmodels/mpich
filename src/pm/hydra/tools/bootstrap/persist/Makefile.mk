# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += hydra_persist

hydra_persist_SOURCES = $(top_srcdir)/tools/bootstrap/persist/persist_server.c
hydra_persist_CFLAGS = $(AM_CFLAGS)
hydra_persist_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_persist_LDADD = -lhydra $(external_libs)
hydra_persist_DEPENDENCIES = libhydra.la

libhydra_la_SOURCES += $(top_srcdir)/tools/bootstrap/persist/persist_init.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_launch.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_wait.c
