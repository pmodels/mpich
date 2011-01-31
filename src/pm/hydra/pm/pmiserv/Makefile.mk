# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/pm/utils

bin_PROGRAMS += hydra_pmi_proxy

hydra_pmi_proxy_SOURCES = $(top_srcdir)/pm/pmiserv/pmip.c \
	$(top_srcdir)/pm/pmiserv/pmip_cb.c \
	$(top_srcdir)/pm/pmiserv/pmip_utils.c \
	$(top_srcdir)/pm/pmiserv/pmip_pmi_v1.c \
	$(top_srcdir)/pm/pmiserv/pmip_pmi_v2.c \
	$(top_srcdir)/pm/pmiserv/common.c \
	$(top_srcdir)/pm/pmiserv/pmi_v2_common.c
hydra_pmi_proxy_CFLAGS = $(AM_CFLAGS)
hydra_pmi_proxy_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_pmi_proxy_LDADD = -lhydra $(external_libs)
hydra_pmi_proxy_DEPENDENCIES = libhydra.la

libpm_la_SOURCES += $(top_srcdir)/pm/pmiserv/pmiserv_pmi.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v1.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmi_v2.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_pmci.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_cb.c \
	$(top_srcdir)/pm/pmiserv/pmiserv_utils.c \
	$(top_srcdir)/pm/pmiserv/common.c \
	$(top_srcdir)/pm/pmiserv/pmi_v2_common.c
