# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/pm/utils

bin_PROGRAMS += pmi_proxy

pmi_proxy_SOURCES = $(top_srcdir)/pm/pmiserv/pmip.c \
	$(top_srcdir)/pm/pmiserv/pmip_cb.c \
	$(top_srcdir)/pm/pmiserv/pmip_utils.c \
	$(top_srcdir)/pm/pmiserv/pmi_common.c
pmi_proxy_CFLAGS = $(AM_CFLAGS)
pmi_proxy_LDFLAGS = $(external_ldflags)
pmi_proxy_LDADD = -lhydra $(external_libs)

libpm_la_SOURCES += $(top_srcdir)/pm/pmiserv/pmiserv_pmi.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v1.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v2.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmci.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_cb.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_utils.c \
	$(top_srcdir)/pm/pmiserv/pmi_common.c
