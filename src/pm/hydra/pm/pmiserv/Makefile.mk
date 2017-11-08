## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/pm/utils

noinst_HEADERS +=              \
    pm/pmiserv/common.h        \
    pm/pmiserv/pmi_v2_common.h \
    pm/pmiserv/pmip.h          \
    pm/pmiserv/pmip_pmi.h      \
    pm/pmiserv/pmiserv.h       \
    pm/pmiserv/pmiserv_pmi.h   \
    pm/pmiserv/pmiserv_utils.h

bin_PROGRAMS += hydra_pmi_proxy

hydra_pmi_proxy_SOURCES = pm/pmiserv/pmip.c \
	pm/pmiserv/pmip_cb.c \
	pm/pmiserv/pmip_utils.c \
	pm/pmiserv/pmip_pmi_v1.c \
	pm/pmiserv/pmip_pmi_v2.c \
	pm/pmiserv/common.c \
	pm/pmiserv/pmi_v2_common.c
hydra_pmi_proxy_CFLAGS = $(AM_CFLAGS)
hydra_pmi_proxy_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_pmi_proxy_LDADD = -lhydra $(external_libs)
hydra_pmi_proxy_DEPENDENCIES = libhydra.la

libpm_la_SOURCES += pm/pmiserv/pmiserv_pmi.c \
	pm/pmiserv/pmiserv_pmi_v1.c \
	pm/pmiserv/pmiserv_pmi_v2.c \
	pm/pmiserv/pmiserv_pmci.c \
	pm/pmiserv/pmiserv_cb.c \
	pm/pmiserv/pmiserv_utils.c \
	pm/pmiserv/common.c \
	pm/pmiserv/pmi_v2_common.c

doc1_src_txt += pm/pmiserv/hydra_pmi_proxy.txt
