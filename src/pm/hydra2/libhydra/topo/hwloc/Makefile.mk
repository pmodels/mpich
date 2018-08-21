## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

libhydra_la_SOURCES += libhydra/topo/hwloc/hydra_topo_hwloc.c

noinst_HEADERS += libhydra/topo/hwloc/hydra_topo_hwloc.h

if hydra_use_embedded_hwloc
AM_CPPFLAGS += @HWLOC_EMBEDDED_CPPFLAGS@
AM_CFLAGS += @HWLOC_EMBEDDED_CFLAGS@

# Append hwloc to the external subdirs, so it gets built first
external_subdirs += libhydra/topo/hwloc/hwloc
external_dist_subdirs += libhydra/topo/hwloc/hwloc
external_libs += @HWLOC_EMBEDDED_LDADD@ @HWLOC_EMBEDDED_LIBS@
endif
