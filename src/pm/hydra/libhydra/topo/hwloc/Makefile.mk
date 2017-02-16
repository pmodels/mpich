## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

libhydra_la_SOURCES += libhydra/topo/hwloc/hydra_topo_hwloc.c

noinst_HEADERS += libhydra/topo/hwloc/hydra_topo_hwloc.h

if hydra_use_embedded_hwloc
AM_CPPFLAGS += -I$(top_srcdir)/libhydra/topo/hwloc/hwloc/include \
	-I$(top_builddir)/libhydra/topo/hwloc/hwloc/include

# Append hwloc to the external subdirs, so it gets built first
external_subdirs += libhydra/topo/hwloc/hwloc
external_dist_subdirs += libhydra/topo/hwloc/hwloc
external_ldflags += -L$(top_builddir)/libhydra/topo/hwloc/hwloc/src
external_libs += -lhwloc_embedded
endif
