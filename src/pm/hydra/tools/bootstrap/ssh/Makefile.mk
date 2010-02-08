# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_la_SOURCES += $(top_srcdir)/tools/bootstrap/ssh/ssh_init.c \
	$(top_srcdir)/tools/bootstrap/ssh/ssh_launch.c \
	$(top_srcdir)/tools/bootstrap/ssh/ssh_finalize.c
