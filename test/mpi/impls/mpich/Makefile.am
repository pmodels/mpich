##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist.in

static_subdirs = mpi_t coll comm misc ulfm session info
SUBDIRS      = $(static_subdirs) $(threadsdir) $(cudadir) $(hipdir)
