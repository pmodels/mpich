# -*- Mode: Makefile; -*-
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS += -I$(top_srcdir)/tools/topo

libhydra_la_SOURCES += $(top_srcdir)/tools/topo/topo.c

if hydra_have_plpa
include tools/topo/plpa/Makefile.mk
endif

if hydra_have_hwloc
include tools/topo/hwloc/Makefile.mk
endif
