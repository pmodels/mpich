# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# mix in the "make testing" rule and other boilerplate
include $(top_srcdir)/Makefile.mtest

ACLOCAL_AMFLAGS = -I confdb

static_subdirs = util attr basic datatype coll comm errhan group info init \
                 pt2pt rma topo errors manual perf
all_lang_subdirs = f77 cxx f90

# DIST_SUBDIRS must be a superset of SUBDIRS, and automake must be able to
# *statically* compute its contents.  The good news is that we can mostly avoid
# duplication because automake is able to "see" into simple variable
# assignments that are not driven by a configure @-substitution variable.
DIST_SUBDIRS = $(static_subdirs) io $(all_lang_subdirs) threads spawn .
SUBDIRS      = $(static_subdirs) $(iodir) $(otherlangs) $(threadsdir) $(spawndir) .

EXTRA_DIST = maint/common.defn maint/f77tof90.in maint/testmerge.in maint/updatefiles testlist.in

DISTCLEANFILES = config.system

## here are automakefile entries for the test/mpi/include dir
noinst_HEADERS = include/mpitest.h include/mpitestcxx.h include/mpithreadtest.h
nodist_noinst_HEADERS = include/mpitestconf.h
