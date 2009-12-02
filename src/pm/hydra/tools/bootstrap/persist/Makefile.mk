# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# libhydra_a_SOURCES += $(top_srcdir)/tools/bootstrap/ssh/ssh_init.c \
# 	$(top_srcdir)/tools/bootstrap/ssh/ssh_launch.c

bin_PROGRAMS += hydserv

hydserv_SOURCES = $(top_srcdir)/tools/bootstrap/persist/hydserv.c
hydserv_LDADD = libhydra.a $(external_libs)
hydserv_LDFLAGS = $(external_ldflags)
