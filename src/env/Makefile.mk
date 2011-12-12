## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

bin_SCRIPTS +=           \
    src/env/mpicc        \
    src/env/parkill

bin_PROGRAMS += src/env/mpich2version

src_env_mpich2version_SOURCES = src/env/mpich2version.c
src_env_mpich2version_LDADD = lib/lib@MPILIBNAME@.la

sysconf_DATA += src/env/mpicc.conf

if BUILD_F77_BINDING
bin_SCRIPTS += src/env/mpif77
sysconf_DATA += src/env/mpif77.conf
endif BUILD_F77_BINDING

if BUILD_F90_LIB
bin_SCRIPTS += src/env/mpif90
sysconf_DATA += src/env/mpif90.conf
endif BUILD_F90_LIB

if BUILD_CXX_LIB
bin_SCRIPTS += src/env/mpicxx
sysconf_DATA += src/env/mpicxx.conf
endif BUILD_CXX_LIB

# create a local copy of the compiler wrapper that will actually be installed
if BUILD_BASH_SCRIPTS
$(top_builddir)/src/env/mpicc: $(top_builddir)/src/env/mpicc.bash
	cp -p $? $@
$(top_builddir)/src/env/mpicxx: $(top_builddir)/src/env/mpicxx.bash
	cp -p $? $@
$(top_builddir)/src/env/mpif77: $(top_builddir)/src/env/mpif77.bash
	cp -p $? $@
$(top_builddir)/src/env/mpif90: $(top_builddir)/src/env/mpif90.bash
	cp -p $? $@
else !BUILD_BASH_SCRIPTS
$(top_builddir)/src/env/mpicc: $(top_builddir)/src/env/mpicc.sh
	cp -p $? $@
$(top_builddir)/src/env/mpicxx: $(top_builddir)/src/env/mpicxx.sh
	cp -p $? $@
$(top_builddir)/src/env/mpif77: $(top_builddir)/src/env/mpif77.sh
	cp -p $? $@
$(top_builddir)/src/env/mpif90: $(top_builddir)/src/env/mpif90.sh
	cp -p $? $@
endif !BUILD_BASH_SCRIPTS

DISTCLEANFILES += $(top_builddir)/src/env/cc_shlib.conf  \
                  $(top_builddir)/src/env/cxx_shlib.conf \
                  $(top_builddir)/src/env/f77_shlib.conf \
                  $(top_builddir)/src/env/fc_shlib.conf  \
                  $(top_builddir)/src/env/mpicc          \
                  $(top_builddir)/src/env/mpicxx         \
                  $(top_builddir)/src/env/mpif77         \
                  $(top_builddir)/src/env/mpif90

# FIXME do this the automake way
#doc_sources = mpicc.txt mpif77.txt mpicxx.txt mpif90.txt mpiexec.txt
#DOCDESTDIRS = html:www/www1,man:man/man1,latex:doc/refman
#doc_HTML_SOURCES  = ${doc_sources}
#doc_MAN_SOURCES   = ${doc_sources}
#doc_LATEX_SOURCES = ${doc_sources}

