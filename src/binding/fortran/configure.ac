##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

dnl define a m4 macro so confdb functions can tell
m4_define([mpich_fortran_configure], [yes])

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

PAC_INIT_ALL_FLAGS(WRAPPER)

# When build from MPICH, --with-mpi=mpich to build native mpich ABI, --with-mpi=abi to build MPI ABI
# When build stand-alone, set --with-mpi to path. TODO: check for system mpi.
AC_ARG_WITH(mpi, [AS_HELP_STRING([--with-mpi=dir],
            [Specify the path where MPI is installed.])])

# This option is ignored when build FROM_MPICH.
# When build stand-alone, this option can be used to pick an abi if both libmpi and libmpi_abi are available
# User also can set "$mpi_libs" to e.g. `-lmpi` or `-lmpi_abi` directly.
AC_ARG_ENABLE(mpi-abi,
        AS_HELP_STRING([--enable-mpi-abi], [Build with libmpi_abi]),,
        enable_mpi_abi=no)

case "$enable_mpi_abi" in
  yes|mpi|mpi,mpich)
    enable_mpi_abi=yes
    ;;
  no|mpich|both|dual|mpich,mpi)
    enable_mpi_abi=no
    ;;
  *)
    AC_MSG_ERROR([Error option --enable-mpi-abi=$enable_mpi_abi])
    ;;
esac


# $mpi_h is needed for python scripts gen_binding_{f77,f90,f08}.py
mpi_h=

AC_CONFIG_HEADERS([mpifort_config.h])

AC_ARG_VAR([MPIFCLIBNAME],[can be used to override the name of the MPI fortran library (default: "mpifort")])
if test -z "$MPIFCLIBNAME" ; then
    MPIFCLIBNAME="mpifort" ;
fi
export MPIFCLIBNAME
AC_SUBST(MPIFCLIBNAME)

# Find a C compiler.
# We also need to do this before the F77 and FC test to ensure that we
# find the C preprocessor reliably.
AC_PROG_CC
AM_PROG_CC_C_O

if test "$FROM_MPICH" = "yes" ; then
    PAC_APPEND_FLAG([-I${main_top_builddir}/src/include], [CPPFLAGS])
    if test "$WITH_MPI" = "mpich" ; then
        mpi_h="$main_top_srcdir/src/include/mpi.h.in"
    elif test "$WITH_MPI" = "abi" ; then
        mpi_h="$main_top_srcdir/src/binding/abi/mpi_abi.h"
        PAC_APPEND_FLAG([-I${main_top_srcdir}/src/binding/abi], [CPPFLAGS])
    else
        AC_MSG_ERROR([$FROM_MPICH is set but $WITH_MPI is not set!])
    fi
    if test -z "$mpi_libs" ; then
        AC_MSG_ERROR([MPICH configure need export mpi_libs])
    fi
else
    if test -z "$with_mpi" ; then
        # get a C compiler that can find mpi
        mpicc=mpicc
        if test "$enable_mpi_abi" = "yes" ; then
            mpicc=mpicc_abi
        fi
        AC_PATH_PROG([MPICC], [$mpicc], [no])

        AC_MSG_CHECKING([path to mpi])
        if test "$MPICC" = "no"; then
            MPICC="$CC"
        fi
        # trick to get it spit out the path to mpi.h
        [mpi_h=`echo '#include <mpi.h>' | $MPICC -M -x c - | grep -oE '/[^ ]*/mpi\.h' | head -n1`]
        with_mpi=${mpi_h%/include/mpi.h}
        AC_MSG_RESULT([$with_mpi])
    fi

    if ! test -f "$with_mpi/include/mpi.h" ; then
        AC_MSG_ERROR([mpi.h not found in $with_mpi/include/])
    fi
    PAC_APPEND_FLAG(-I${with_mpi}/include, [CPPFLAGS])
    PAC_APPEND_FLAG(-L${with_mpi}/lib, [LDFLAGS])

    # Check whether we are testing MPI derived from MPICH
    MPI_IS_MPICH=no
    AC_MSG_CHECKING([Is the MPI derived from MPICH])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[return 1 + MPICH;])],
        [MPI_IS_MPICH=yes],[MPI_IS_MPICH=no])
    AC_MSG_RESULT($MPI_IS_MPICH)

    WITH_MPI=

    AC_CHECK_LIB([mpi], [MPI_Init], [has_mpi=yes], [has_mpi=no])
    AC_CHECK_LIB([mpi_abi], [MPI_Init], [has_mpi_abi=yes], [has_mpi_abi=no])
    if test "$enable_mpi_abi" = "yes" ; then
        if test "$has_mpi_abi" = "yes" ; then
            WITH_MPI="abi"
            PAC_LIBS_ADD([-lmpi_abi])
        else
            AC_MSG_ERROR([Option enable-mpi-abi is set but libmpi_abi not found!])
        fi
    else
        if test "$has_mpi" = "yes" ; then
            PAC_LIBS_ADD([-lmpi])
        elif test "$has_mpi_abi" = "yes" ; then
            WITH_MPI="abi"
            PAC_LIBS_ADD([-lmpi_abi])
        else
            AC_MSG_ERROR([Neither libmpi nor libmpi_abi is found!])
        fi

        if test "$WITH_MPI" = "abi" ; then
            AC_CHECK_LIB([pmpi_abi], [PMPI_Init])
        else
            AC_CHECK_LIB([pmpi], [PMPI_Init])
        fi
    fi

    mpi_h="$with_mpi/include/mpi.h"
    if test -f "$with_mpi/include/mpi_abi.h" -a "$WITH_MPI" = "abi"; then
        mpi_h="$with_mpi/include/mpi_abi.h"
    fi

fi
AC_SUBST([mpi_libs])

# ---------------------------------------------
# Substitution macros for mpifort

# User-provided flags
AC_SUBST(MPICH_MPIFORT_CPPFLAGS)
AC_SUBST(MPICH_MPIFORT_FCFLAGS)
AC_SUBST(MPICH_MPIFORT_LDFLAGS)
AC_SUBST(MPICH_MPIFORT_LIBS)

AC_SUBST(MPICH_MPIF77_CPPFLAGS)
AC_SUBST(MPICH_MPIF77_FCFLAGS)
AC_SUBST(MPICH_MPIF77_LDFLAGS)
AC_SUBST(MPICH_MPIF77_LIBS)

# Check that the Fortran compiler will allow us to pass arguments
# of different types (e.g., for MPI_Send)
PAC_PROG_F77_MISMATCHED_ARGS(addarg,yes)
if test "X$addarg" != "X" ; then
    # Code using mpif.h interface will likely need this flag to compile.
    # Code with `use mpi` or `use mpi_f08` do not need this flag.
    # Add the flag to mpif77 wrappers.
    WRAPPER_EXTRA_F77_FLAGS="$addarg"
fi
AC_SUBST(WRAPPER_EXTRA_F77_FLAGS)

AC_ARG_ENABLE(interlib-deps,[
    AS_HELP_STRING([--enable-interlib-deps - Enable interlibrary dependencies])],,enable_interlib_deps=yes)
if test -z "$INTERLIB_DEPS" ; then
    INTERLIB_DEPS=yes
    if test "X$enable_shared" = "Xno" -o "$deplibs_check_method" != "pass_all" -o "$enable_interlib_deps" = "no" ; then
        INTERLIB_DEPS=no
    fi
    # Used when INTERLIB_DEPS is "no"
    mpi_dependency_libs="$mpi_libs"
fi
AC_SUBST(INTERLIB_DEPS)

WRAPPER_DEPENDENCY_LDFLAGS="$mpi_dependency_libs"
AC_SUBST(WRAPPER_DEPENDENCY_LDFLAGS)

# generate rpath-related macros, needed for substitute with_wrapper_dl_type
fc_shlib_conf=fc_shlib.conf
PAC_COMPILER_SHLIB_FLAGS([FC],[$fc_shlib_conf])
AC_SUBST_FILE([fc_shlib_conf])



# ---------------------------------------------

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
