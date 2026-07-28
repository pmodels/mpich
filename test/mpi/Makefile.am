##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mix in the "make testing" rule and other boilerplate
include $(top_srcdir)/Makefile_single.mtest
include $(top_srcdir)/Makefile_mpitests.mtest

ACLOCAL_AMFLAGS = -I confdb

static_subdirs = util attr basic datatype coll comm errhan group info init \
                 pt2pt rma topo errors manual perf mpi_t impls ckpoint dtpools \
                 part session
all_lang_subdirs = f77 cxx f90 f08

SUBDIRS      = $(static_subdirs) $(iodir) $(otherlangs) $(threadsdir) $(spawndir) $(ckpointdir) $(ftdir) .

DISTCLEANFILES = config.system

## here are automakefile entries for the test/mpi/include dir
noinst_HEADERS = include/mpitest.h include/mpicolltest.h include/mpitestcxx.h include/mpithreadtest.h include/dtypes.h rma/squelch.h errors/rma/win_sync.h
nodist_noinst_HEADERS = include/mpitestconf.h
