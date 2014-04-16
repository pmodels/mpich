## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

bin_SCRIPTS +=           \
    src/env/mpicc        \
    src/env/parkill

bin_PROGRAMS += src/env/mpichversion \
	src/env/mpivars

src_env_mpichversion_SOURCES = src/env/mpichversion.c
src_env_mpichversion_LDADD = lib/lib@MPILIBNAME@.la
if BUILD_PROFILING_LIB
src_env_mpichversion_LDADD += lib/lib@PMPILIBNAME@.la
endif BUILD_PROFILING_LIB

src_env_mpichversion_LDFLAGS = $(mpich_libtool_static_flag)

src_env_mpivars_SOURCES = src/env/mpivars.c
src_env_mpivars_LDADD   = lib/lib@MPILIBNAME@.la
src_env_mpivars_LDFLAGS = $(mpich_libtool_static_flag)
if BUILD_PROFILING_LIB
src_env_mpivars_LDADD += lib/lib@PMPILIBNAME@.la
endif BUILD_PROFILING_LIB

if BUILD_F77_BINDING
if INSTALL_MPIF77
bin_SCRIPTS += src/env/mpif77
endif INSTALL_MPIF77
endif BUILD_F77_BINDING

if BUILD_FC_BINDING
bin_SCRIPTS += src/env/mpifort
endif BUILD_FC_BINDING

if BUILD_CXX_BINDING
bin_SCRIPTS += src/env/mpicxx
endif BUILD_CXX_BINDING

# create a local copy of the compiler wrapper that will actually be installed
if BUILD_BASH_SCRIPTS
src/env/mpicc: $(top_builddir)/src/env/mpicc.bash
	cp -p $? $@
src/env/mpicxx: $(top_builddir)/src/env/mpicxx.bash
	cp -p $? $@
src/env/mpif77: $(top_builddir)/src/env/mpif77.bash
	cp -p $? $@
src/env/mpifort: $(top_builddir)/src/env/mpifort.bash
	cp -p $? $@
else !BUILD_BASH_SCRIPTS
src/env/mpicc: $(top_builddir)/src/env/mpicc.sh
	cp -p $? $@
src/env/mpicxx: $(top_builddir)/src/env/mpicxx.sh
	cp -p $? $@
src/env/mpif77: $(top_builddir)/src/env/mpif77.sh
	cp -p $? $@
src/env/mpifort: $(top_builddir)/src/env/mpifort.sh
	cp -p $? $@
endif !BUILD_BASH_SCRIPTS

DISTCLEANFILES += $(top_builddir)/src/env/cc_shlib.conf  \
                  $(top_builddir)/src/env/cxx_shlib.conf \
                  $(top_builddir)/src/env/f77_shlib.conf \
                  $(top_builddir)/src/env/fc_shlib.conf  \
                  $(top_builddir)/src/env/mpicc          \
                  $(top_builddir)/src/env/mpicxx         \
                  $(top_builddir)/src/env/mpif77         \
                  $(top_builddir)/src/env/mpifort

wrapper_doc_src = src/env/mpicc.txt \
                  src/env/mpif77.txt \
                  src/env/mpicxx.txt \
                  src/env/mpifort.txt \
                  src/env/mpiexec.txt
doc1_src_txt += $(wrapper_doc_src)
EXTRA_DIST += $(wrapper_doc_src)
