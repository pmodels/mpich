##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

m4_include([version.m4])
AC_INIT([mpifort_abi],
        MPICH_VERSION_m4,
        [discuss@mpich.org],
        [mpifort_abi],
        [http://www.mpich.org/])

AC_CONFIG_AUX_DIR([confdb])
AC_CONFIG_MACRO_DIR([confdb])

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects])
AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AM_PROG_CC_C_O
PAC_CHECK_VISIBILITY
AC_SUBST(VISIBILITY_CFLAGS)

AM_PROG_AR

LT_INIT([])
# Non-verbose make by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

# suppress default "-g -O2" from AC_PROG_FC
: ${FCFLAGS=""}
AC_PROG_FC

F77=$FC
FFLAGS=$FCFLAGS
AC_PROG_F77

modincdir=$includedir
AC_SUBST(modincdir)

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
