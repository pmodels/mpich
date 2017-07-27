## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/node

noinst_HEADERS += libhydra/node/hydra_node.h

libhydra_la_SOURCES += \
	libhydra/node/hydra_node.c
