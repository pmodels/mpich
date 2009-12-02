# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += hydserv

hydserv_SOURCES = $(top_srcdir)/tools/bootstrap/persist/persist_server.c
hydserv_LDADD = libhydra.a $(external_libs)
hydserv_LDFLAGS = $(external_ldflags)

libhydra_a_SOURCES += $(top_srcdir)/tools/bootstrap/persist/persist_init.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_launch.c \
	$(top_srcdir)/tools/bootstrap/persist/persist_wait.c
