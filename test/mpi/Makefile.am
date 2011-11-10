# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# mix in the "make testing" rule and other boilerplate
include $(top_srcdir)/Makefile.mtest

ACLOCAL_AMFLAGS = -I ../../confdb

static_subdirs = util attr basic datatype coll comm errhan group info init \
                 pt2pt rma spawn topo errors manual perf
all_lang_subdirs = f77 cxx f90

# DIST_SUBDIRS must be a superset of SUBDIRS, and automake must be able to
# *statically* compute its contents.  The good news is that we can mostly avoid
# duplication because automake is able to "see" into simple variable
# assignments that are not driven by a configure @-substitution variable.
DIST_SUBDIRS = $(static_subdirs) io $(all_lang_subdirs) threads .
SUBDIRS      = $(static_subdirs) $(iodir) $(otherlangs) $(threadsdir) .

EXTRA_DIST = maint/common.defn maint/f77tof90.in maint/testmerge.in maint/updatefiles testlist.in

DISTCLEANFILES = config.system

## here are automakefile entries for the test/mpi/include dir
noinst_HEADERS = include/mpitest.h include/mpitestcxx.h include/mpithreadtest.h
nodist_noinst_HEADERS = include/mpitestconf.h

## FIXME: here's the sketchy "make dist" rule from the Makefile.sm implementation
##dist:	
##	if [ ! -d "@mpich2_top_srcdir@" ] ; then \
##	    echo "The dist target requires that the mpich2 source directory be provided" ; \
##	    exit 1 ; \
##	fi
##	if [ ! -d .tmp ] ; then mkdir .tmp ; else rm -rf .tmp && mkdir .tmp ; fi
##	cd .tmp && svn export https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/test/mpi mpi2test
##	cd .tmp/mpi2test && \
##	    svn export https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk/confdb confdb
##	cd .tmp/mpi2test && \
##	    sed -e 's/AC_CONFIG_AUX_DIR.*/AC_CONFIG_AUX_DIR(confdb)/' \
##	        configure.in > conftmp.in && mv conftmp.in configure.in
##	cd .tmp/mpi2test && maint/updatefiles -mpich2dir=@mpich2_top_srcdir@
##	cd .tmp/mpi2test && @mpich2_top_srcdir@/maint/simplemake Makefile.sm
##	cd .tmp && tar cf mpi2test.tar mpi2test && gzip -9 mpi2test.tar 
##	if [ -s .tmp/mpi2test.tar.gz ] ; then \
##	    mv .tmp/mpi2test.tar.gz . ; rm -rf .tmp ; \
##	else echo "Could not create tarfile for test suite"; exit 1 ; fi
