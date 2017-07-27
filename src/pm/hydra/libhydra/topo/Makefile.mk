## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/topo

noinst_HEADERS += libhydra/topo/hydra_topo.h \
	libhydra/topo/hydra_topo_internal.h

libhydra_la_SOURCES += libhydra/topo/hydra_topo.c

if hydra_have_hwloc
include libhydra/topo/hwloc/Makefile.mk
endif
