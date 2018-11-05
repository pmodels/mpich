#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([dtpools], [0.0], [discuss@mpich.org])
AC_CONFIG_SRCDIR([include/dtpools.h])

AC_CONFIG_HEADER(dtpoolsconf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef DTPOOLSCONF_H_INCLUDED
#define DTPOOLSCONF_H_INCLUDED
])

AH_BOTTOM([
#endif /* !defined(DTPOOLSCONF_H_INCLUDED) */
])

# Ensure we have an MPI compiler wrapper
if test -n "$MPICC" ; then
  CC=$MPICC
fi

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])
AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT

AC_ARG_VAR([DTP_NUM_OBJS],
           [used to limit the number of objects to test from each pool])

# begin framework MPI datatype initialization
AC_ARG_VAR([DTP_DATATYPE_FILE],
           [used to pass MPI datatypes to the DTPools framework (default: "basictypelist.txt")])

# we must have a valid file containing MPI datatypes for framework initialization
DTP_DATATYPE_FILE=${DTP_DATATYPE_FILE:-$srcdir/basictypelist.txt}
if test ! -f "$DTP_DATATYPE_FILE" ; then
   AC_MSG_ERROR([Need a valid file for MPI datatypes.])
fi

# get comma separated list of MPI datatypes for static datatype array initialization in src/dtpools.c
DTP_DATATYPES=$(types=""; while read -r line; do types+="$line"; done < $DTP_DATATYPE_FILE; echo "$types")
AC_SUBST([DTP_DATATYPES])
# end of framework initialization

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h limits.h stdint.h sys/types.h],,
                 [AC_MSG_ERROR([Cannot find headers.])],)

# Must be able to find mpi.h
MPI_H_INCLUDE="-I${master_top_builddir}/src/include"
AC_SUBST([MPI_H_INCLUDE])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
