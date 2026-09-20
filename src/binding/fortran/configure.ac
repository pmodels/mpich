##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

m4_include([version.m4])

AC_INIT([MPIFORT], MPICH_VERSION_m4, [discuss@mpich.org], [mpifort], [http://www.mpich.org/])

MPICH_VERSION=MPICH_VERSION_m4
AC_SUBST([MPICH_VERSION])

## Enable creation of libtool-style versioning or no versioning
AC_ARG_ENABLE(versioning,
    [AS_HELP_STRING([--enable-versioning],[Enable library versioning])],,
    [enable_versioning=yes])

if test "$enable_versioning" = "yes" ; then
    ABIVERSIONFLAGS="-version-info libmpi_so_version_m4"
else
    ABIVERSIONFLAGS="-avoid-version"
fi
AC_SUBST(ABIVERSIONFLAGS)

AC_CONFIG_AUX_DIR(confdb)
AC_CONFIG_MACRO_DIR(confdb)
AM_INIT_AUTOMAKE([-Wall -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects no-dist])
AM_MAINTAINER_MODE([disable])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Find a C compiler.
# We also need to do this before the F77 and FC test to ensure that we
# find the C preprocessor reliably.
AC_PROG_CC
AM_PROG_CC_C_O

AM_CONDITIONAL([INSTALL_MPIF90],[true])
AM_CONDITIONAL([INSTALL_MPIF77],[true])

PAC_CHECK_BASH
AM_CONDITIONAL([BUILD_BASH_SCRIPTS], [test "x$pac_cv_bash_works" = xyes])

AC_CONFIG_FILES([Makefile \
                 mpif_h/mpi_fortran.h \
                 env/mpifort.sh \
                 env/mpifort.bash \
                 localdefs])
AC_OUTPUT
