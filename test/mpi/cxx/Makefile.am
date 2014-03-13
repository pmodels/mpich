# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_cxx.mtest

EXTRA_DIST = testlist.in

static_subdirs = util attr pt2pt init info comm coll datatype errhan topo
SUBDIRS      = $(static_subdirs) $(iodir) $(rmadir) $(spawndir)
DIST_SUBDIRS = $(static_subdirs) io rma spawn
