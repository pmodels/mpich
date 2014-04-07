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
                 pt2pt rma topo errors manual perf mpi_t impls ckpoint ft
all_lang_subdirs = f77 cxx f90 f08

# DIST_SUBDIRS must be a superset of SUBDIRS, and automake must be able to
# *statically* compute its contents.  The good news is that we can mostly avoid
# duplication because automake is able to "see" into simple variable
# assignments that are not driven by a configure @-substitution variable.
DIST_SUBDIRS = $(static_subdirs) io $(all_lang_subdirs) threads spawn .
SUBDIRS      = $(static_subdirs) $(iodir) $(otherlangs) $(threadsdir) $(spawndir) $(ckpointdir) $(ftdir) .

EXTRA_DIST = maint/common.defn maint/f77tof90.in maint/testmerge.in maint/updatefiles testlist.in

DISTCLEANFILES = config.system

## here are automakefile entries for the test/mpi/include dir
noinst_HEADERS = include/mpitest.h include/mpitestcxx.h include/mpithreadtest.h include/dtypes.h rma/squelch.h errors/rma/win_sync.h
nodist_noinst_HEADERS = include/mpitestconf.h

# Need to patch some things up:
# Make sure to disable rebuilding the autotools related files - the
# receiver of this distribution may not have the necessary tools and
# should not need them.  Finally, the distributed configure must disable
# maintainer targets by default - otherwise, the unsuspecting user will
# see automake et al attempt to rebuild the autotools, which is likely to
# fail unless the user has the correct versions of all of the tools
dist-hook:
	cd $(distdir) && \
	    sed -e 's/AM_MAINTAINER_MODE.*/AM_MAINTAINER_MODE([disable])/' \
	        configure.ac > conftmp.ac && mv conftmp.ac configure.ac
	cd $(distdir) && $(AUTOMAKE) --add-missing
	cd $(distdir) && $(AUTOMAKE) --foreign
	cd $(distdir) && $(ACLOCAL) -Iconfdb
	cd $(distdir) && $(AUTOCONF) -Iconfdb
	cd $(distdir) && $(AUTOHEADER) -Iconfdb
