# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += hydserv

hydserv_SOURCES = $(top_srcdir)/tools/bootstrap/persist/persist_server.c
hydserv_CFLAGS = $(AM_CFLAGS)
hydserv_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydserv_LDADD = -lhydra $(external_libs)
hydserv_DEPENDENCIES = libhydra.la

libhydra_la_SOURCES += $(top_srcdir)/tools/bootstrap/persist/persist_init.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_launch.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_wait.c
