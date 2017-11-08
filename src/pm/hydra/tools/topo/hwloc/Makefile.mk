## -*- Mode: Makefile; -*-
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

libhydra_la_SOURCES += tools/topo/hwloc/topo_hwloc.c

noinst_HEADERS += tools/topo/hwloc/topo_hwloc.h

if hydra_use_embedded_hwloc
AM_CPPFLAGS += -I$(top_srcdir)/tools/topo/hwloc/hwloc/include \
	-I$(top_builddir)/tools/topo/hwloc/hwloc/include

# Append hwloc to the external subdirs, so it gets built first
external_subdirs += tools/topo/hwloc/hwloc
external_dist_subdirs += tools/topo/hwloc/hwloc
external_ldflags += -L$(top_builddir)/tools/topo/hwloc/hwloc/src
external_libs += -lhwloc_embedded
endif
