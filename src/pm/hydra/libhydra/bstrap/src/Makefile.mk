## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/src

libhydra_la_SOURCES += \
	libhydra/bstrap/src/hydra_bstrap.c

noinst_HEADERS += \
	libhydra/bstrap/src/hydra_bstrap_common.h \
	libhydra/bstrap/src/hydra_bstrap.h

bin_PROGRAMS += hydra_bstrap_proxy

hydra_bstrap_proxy_SOURCES = \
	libhydra/bstrap/src/hydra_bstrap_proxy.c
hydra_bstrap_proxy_CFLAGS = $(AM_CFLAGS)
hydra_bstrap_proxy_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_bstrap_proxy_LDADD = -lhydra $(external_libs)
hydra_bstrap_proxy_DEPENDENCIES = libhydra.la
