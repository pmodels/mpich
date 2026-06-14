##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_f90.mtest

EXTRA_DIST = testlist.in

static_subdirs = timer attr coll datatype pt2pt info comm topo ext init \
		 misc
SUBDIRS = $(static_subdirs) $(rmadir) $(spawndir) $(iodir) $(f90profile)
DIST_SUBDIRS = $(static_subdirs) rma spawn io
