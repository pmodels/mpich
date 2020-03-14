##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/topo

noinst_HEADERS += libhydra/topo/hydra_topo.h \
	libhydra/topo/hydra_topo_internal.h

libhydra_la_SOURCES += libhydra/topo/hydra_topo.c

if HYDRA_HAVE_HWLOC
include libhydra/topo/hwloc/Makefile.mk
endif
