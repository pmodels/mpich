## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/topo

noinst_HEADERS += tools/topo/topo.h

libhydra_la_SOURCES += tools/topo/topo.c

if hydra_have_hwloc
include tools/topo/hwloc/Makefile.mk
endif
