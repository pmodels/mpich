# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# mix in the "make testing" rule and other boilerplate
include $(top_srcdir)/Makefile.utest

ACLOCAL_AMFLAGS = -I confdb

static_subdirs = utest str

# DIST_SUBDIRS must be a superset of SUBDIRS, and automake must be able to
# *statically* compute its contents.  The good news is that we can mostly avoid
# duplication because automake is able to "see" into simple variable
# assignments that are not driven by a configure @-substitution variable.
DIST_SUBDIRS = $(static_subdirs) .
SUBDIRS      = $(static_subdirs) .

# EXTRA_DIST = maint/common.defn maint/f77tof90 maint/testmerge.in maint/updatefiles testlist.in

DISTCLEANFILES = config.system

## here are automakefile entries for the test/mpi/include dir
noinst_HEADERS = include/utest.h
nodist_noinst_HEADERS = include/mpltestconf.h

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
