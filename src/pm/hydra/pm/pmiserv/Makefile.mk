# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

bin_PROGRAMS += pmi_proxy

pmi_proxy_SOURCES = $(top_srcdir)/pm/pmiserv/pmi_proxy.c \
	$(top_srcdir)/pm/pmiserv/pmi_proxy_cb.c \
	$(top_srcdir)/pm/pmiserv/pmi_proxy_utils.c
pmi_proxy_LDADD = libhydra.a $(external_libs)
pmi_proxy_LDFLAGS = $(external_ldflags)

libpm_a_SOURCES += $(top_srcdir)/pm/pmiserv/pmi_handle.c \
	$(top_srcdir)/pm/pmiserv/pmi_handle_common.c \
	$(top_srcdir)/pm/pmiserv/pmi_handle_v1.c \
	$(top_srcdir)/pm/pmiserv/pmi_handle_v2.c \
	$(top_srcdir)/pm/pmiserv/pmi_serv_cb.c \
	$(top_srcdir)/pm/pmiserv/pmi_serv_finalize.c \
	$(top_srcdir)/pm/pmiserv/pmi_serv_launch.c \
	$(top_srcdir)/pm/pmiserv/pmi_serv_utils.c
