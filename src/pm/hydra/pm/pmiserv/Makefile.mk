# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/pm/utils

bin_PROGRAMS += pmi_proxy

pmi_proxy_SOURCES = $(top_srcdir)/pm/pmiserv/pmip.c \
	$(top_srcdir)/pm/pmiserv/pmip_cb.c \
	$(top_srcdir)/pm/pmiserv/pmip_utils.c
pmi_proxy_LDADD = libhydra.a libpm.a $(external_libs)
pmi_proxy_LDFLAGS = $(external_ldflags)

libpm_a_SOURCES += $(top_srcdir)/pm/pmiserv/pmiserv_pmi.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v1.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v2.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmci.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_cb.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_utils.c
