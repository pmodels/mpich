##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/topo

noinst_HEADERS += lib/tools/topo/topo.h

libhydra_la_SOURCES += lib/tools/topo/topo.c

if HYDRA_HAVE_HWLOC
include lib/tools/topo/hwloc/Makefile.mk
endif
