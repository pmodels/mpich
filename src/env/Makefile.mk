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

DISTCLEANFILES += ./src/env/cc_shlib.conf ./src/env/cxx_shlib.conf \
                  ./src/env/f77_shlib.conf ./src/env/fc_shlib.conf

# FIXME do this the automake way
#doc_sources = mpicc.txt mpif77.txt mpicxx.txt mpif90.txt mpiexec.txt
#DOCDESTDIRS = html:www/www1,man:man/man1,latex:doc/refman
#doc_HTML_SOURCES  = ${doc_sources}
#doc_MAN_SOURCES   = ${doc_sources}
#doc_LATEX_SOURCES = ${doc_sources}

