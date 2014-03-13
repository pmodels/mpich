# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f90.mtest

EXTRA_DIST = testlist.in

static_subdirs = util timer attr coll datatype pt2pt info comm topo ext init \
		 misc f90types profile
SUBDIRS = $(static_subdirs) $(rmadir) $(spawndir) $(iodir)
DIST_SUBDIRS = $(static_subdirs) rma spawn io
