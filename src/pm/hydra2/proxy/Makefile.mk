## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/proxy

noinst_HEADERS += \
	proxy/proxy.h

bin_PROGRAMS += hydra_pmi_proxy

hydra_pmi_proxy_SOURCES = \
	proxy/proxy.c \
	proxy/proxy_cb.c \
	proxy/proxy_pmi.c \
	proxy/proxy_pmi_cb.c

hydra_pmi_proxy_CFLAGS = $(AM_CFLAGS)
hydra_pmi_proxy_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_pmi_proxy_LDADD = -lhydra $(external_libs)
hydra_pmi_proxy_DEPENDENCIES = libhydra.la @mpl_lib@
