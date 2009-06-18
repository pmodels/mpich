# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

libhydra_a_SOURCES += $(top_builddir)/css/src/cssi_init.c \
	$(top_srcdir)/css/src/cssi_finalize.c \
	$(top_srcdir)/css/src/cssi_query_string.c
