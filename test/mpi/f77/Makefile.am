##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist.in

static_subdirs = attr datatype coll pt2pt info init comm topo ext @f77profile@
SUBDIRS = $(static_subdirs) $(spawndir) $(iodir) $(rmadir)
DIST_SUBDIRS = $(static_subdirs) spawn io rma
