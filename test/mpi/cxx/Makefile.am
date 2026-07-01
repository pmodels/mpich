##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_cxx.mtest

EXTRA_DIST = testlist.in

static_subdirs = attr pt2pt init info comm coll datatype errhan topo
SUBDIRS      = $(static_subdirs) $(iodir) $(rmadir) $(spawndir)
DIST_SUBDIRS = $(static_subdirs) io rma spawn
