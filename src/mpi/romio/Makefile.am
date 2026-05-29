##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

## TODO: need to write an automakefile that handles two primary cases:
## 1) that ROMIO is being embedded within the MPI library, as in MPICH or Open
##    MPI
## 2) that ROMIO is being built standalone, old-school style.  This case is
##    basically unused in modern practice.

# help autoreconf and friends realize where the macros live
ACLOCAL_AMFLAGS = -I confdb

# empty variable initializations so that later code can append (+=)
include_HEADERS = 
nodist_include_HEADERS =
noinst_HEADERS = 
AM_CFLAGS = @VISIBILITY_CFLAGS@
AM_CPPFLAGS =
EXTRA_DIST =
SUFFIXES = 

external_subdirs = @mpl_srcdir@
external_dist_subdirs = @mpl_dist_srcdir@
external_includes = @mpl_includedir@
external_ldflags =
external_libs = @mpl_lib@

# ------------------------------------------------------------------------
# variables to be populated by the included Makefile.mk fragments:

# These are files that contain MPI routines (e.g., MPI_File_open).
# In MPICH these will have an MPI_ and a PMPI_ version.  Other implementations
# (like OMPI) only want these to be MPI_ routines, possibly with some
# name-shifting prefix.
romio_mpi_sources = 

# regular old source files that implement ROMIO, such as ADIO code
romio_other_sources =

# code that may need to be "up" called from the MPI library and/or is
# MPI-implementation-specific in some way
glue_sources = 

# ------------------------------------------------------------------------
# when building under MPICH we must be able to find mpi.h
AM_CPPFLAGS += $(MPI_H_INCLUDE)

# ------------------------------------------------------------------------
# handle the "include" directory here
AM_CPPFLAGS += -I$(top_builddir)/include -I$(top_srcdir)/include $(external_includes)
# nodist_ b/c these are created by config.status and should not be distributed
if INSTALL_HEADERS
nodist_include_HEADERS += include/mpio.h
endif

# ------------------------------------------------------------------------

SUBDIRS = $(external_subdirs) . test
DIST_SUBDIRS = test test-internal $(external_dist_subdirs)

# for the sake of parallel make and avoiding an excessive number of convenience
# libs, we use a subdir automake fragment strategy
include mpi-io/Makefile.mk
include adio/Makefile.mk

EXTRA_DIST += autogen.sh

if BUILD_ROMIO_EMBEDDED
# Build a libtool convenience library that the enclosing MPI implementation can
# use by adding it to the right _LIBADD variable.
noinst_LTLIBRARIES = libromio.la
libromio_la_SOURCES = $(romio_mpi_sources) $(romio_other_sources) $(glue_sources)

## NOTE: ROMIO's old build system builds a bunch of _foo.o objects that contain
## PMPI_ implementations as well as calls to only other PMPI routines.  In
## MPICH, these are the objects that need to go into libmpi, while the foo.o
## objects should go into libpmpi.  Furthermore, the -D option for ROMIO's
## source files is different and inverted (in the boolean sense) compared with
## MPICH's definition.  And ROMIO was dumping all of the symbols into the main
## libmpi library, regardless of the separate profiling library's existence.
##
## Annoying, right?
if BUILD_PROFILING_LIB
# The current best strategy for now is to build the PMPI symbols as a separate
# convenience lib to permit adding the special "-D..." argument for all objects.
# MPICH will then link in both convenience library into libmpi, since it
# won't work very well the other way around.
noinst_LTLIBRARIES += libpromio.la
libpromio_la_SOURCES = $(romio_mpi_sources)
libpromio_la_CPPFLAGS = $(AM_CPPFLAGS) -DMPIO_BUILD_PROFILING 
libpromio_la_LDFLAGS = $(external_ldflags)
libpromio_la_LIBADD = $(external_libs)
else !BUILD_PROFILING_LIB
libromio_la_LDFLAGS = $(external_ldflags)
libromio_la_LIBADD = $(external_libs)
endif !BUILD_PROFILING_LIB

if BUILD_ABI_LIB
noinst_LTLIBRARIES += libromio_abi.la
libromio_abi_la_SOURCES = $(romio_mpi_sources) $(romio_other_sources) $(glue_sources)
libromio_abi_la_CPPFLAGS = $(AM_CPPFLAGS) -DMPICH_BUILD_MPI_ABI

if BUILD_PROFILING_LIB
noinst_LTLIBRARIES += libpromio_abi.la
libpromio_abi_la_SOURCES = $(romio_mpi_sources)
libpromio_abi_la_CPPFLAGS = $(AM_CPPFLAGS) -DMPIO_BUILD_PROFILING -DMPICH_BUILD_MPI_ABI
libpromio_abi_la_LDFLAGS = $(external_ldflags)
libpromio_abi_la_LIBADD = $(external_libs)
endif BUILD_PROFILING_LIB
libromio_abi_la_LDFLAGS = $(external_ldflags)
libromio_abi_la_LIBADD = $(external_libs)
endif BUILD_ABI_LIB

# ---------------------------
else !BUILD_ROMIO_EMBEDDED
lib_LTLIBRARIES = libromio.la
libromio_la_SOURCES = $(romio_mpi_sources) $(romio_other_sources) $(glue_sources)
if BUILD_PROFILING_LIB
libpromio_la_SOURCES = $(romio_mpi_sources)
libpromio_la_CPPFLAGS = $(AM_CPPFLAGS) -DMPIO_BUILD_PROFILING
endif BUILD_PROFILING_LIB

endif

# --------------------------------------------------------------------------
.PHONY: coverage
gcov_sources = $(libmpl_la_SOURCES)
# assumes that these sources were compiled appropriately ("-fprofile-arcs"
# and "-ftest-coverage")
coverage:
	@for file in $(gcov_sources) ; do \
	    dir=`dirname $$file` ; \
	    bname=`basename $$file` ; \
	    aux=`echo $$bname | sed -e 's,\.*$$,,'` ; \
	    echo "( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir )" ; \
	    ( $(GCOV) -b -f -o $$file $$file && mv $${bname}.gcov $$dir ) ; \
	    rm -f *.gcov ; \
	done
	for subdir in $(SUBDIRS) - ; do \
	    if test $$subdir = "-" ; then break ; fi ; \
	    ( cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) coverage ) ; \
	done

# --------------------------------------------------------------------------
# sometimes helpful when debugging macros to see the preprocessed output.
# Also using '-CC' because comments provide useful landmarks

SUFFIXES += .i

.c.i:
	$(COMPILE) -CC -E -o $@ $<

