##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/topo

noinst_HEADERS += lib/tools/topo/topo.h

libhydra_la_SOURCES += lib/tools/topo/topo.c

if HYDRA_HAVE_HWLOC
include lib/tools/topo/hwloc/Makefile.mk
endif
