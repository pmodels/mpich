##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])
AC_INIT([dtpools], [0.0], [discuss@mpich.org])
AC_CONFIG_SRCDIR([include/dtpools.h])

AC_CONFIG_HEADERS([dtpoolsconf.h])
AH_TOP([/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#ifndef DTPOOLSCONF_H_INCLUDED
#define DTPOOLSCONF_H_INCLUDED
])

AH_BOTTOM([
#endif /* !defined(DTPOOLSCONF_H_INCLUDED) */
])

AC_CONFIG_AUX_DIR([confdb])
AC_CONFIG_MACRO_DIR([confdb])

# Ensure we have an MPI compiler wrapper
if test -n "$MPICC" ; then
  CC=$MPICC
fi

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
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

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h limits.h stdint.h sys/types.h],,
                 [AC_MSG_ERROR([Cannot find headers.])],)

# Must be able to find mpi.h
MPI_H_INCLUDE="-I${main_top_builddir}/src/include -I${main_top_srcdir}/src/include"
AC_SUBST([MPI_H_INCLUDE])

# check if we need declarations
AC_CHECK_FUNCS(strdup)
if test "$ac_cv_func_strdup" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
fi

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile])
AC_OUTPUT
