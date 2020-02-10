dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.67)
dnl
dnl aclocal_cache.m4, included by sowing/confdb/aclocal.m4, fixes 
dnl bugs in autoconf caching.
dnl
dnl
dnl Environment variables that affect behavior of the test configure
dnl MPICH_FAST 
dnl
dnl The file name here refers to a file in the source being configured
dnl FIXME this is the old style, needs updating to new style
dnl AC_INIT(include/mpitest.h)
m4_include([version.m4])
AC_INIT([mpl-testsuite],
        MPICH_VERSION_m4,
        [discuss@mpich.org],
        [mpl-testsuite],
        [http://www.mpich.org/])

AC_CONFIG_HEADER(include/mpltestconf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPITESTCONF_H_INCLUDED
#define MPITESTCONF_H_INCLUDED
])
AH_BOTTOM([#endif])

VERSION=MPICH_VERSION_m4
AC_SUBST(VERSION)
AC_CONFIG_AUX_DIR([confdb])
AC_CONFIG_MACRO_DIR([confdb])
dnl
echo "RUNNING CONFIGURE FOR MPL TESTS"

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects])
AM_MAINTAINER_MODE([enable])

# Non-verbose make by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

if test -z "$mpich_top_srcdir" ; then 
    if test -z "$top_srcdir" ; then
       use_top_srcdir=$srcdir   
    else
       use_top_srcdir=$top_srcdir
    fi
    case "$use_top_srcdir" in
    /*) ;;
    *)
        use_top_srcdir=`(cd $use_top_srcdir && pwd)`
        ;;
    esac
    if test -f $use_top_srcdir/../../maint/version.m4 ; then 
        mpich_top_srcdir=`cd $use_top_srcdir && cd ../.. && pwd`
    fi
fi
AC_SUBST(mpich_top_srcdir)

# these (in particular master_top_srcdir) are needed to regenerate
# the f90 files from the f77 files
AC_ARG_VAR([master_top_builddir],[path to the MPICH top-level build directory (if present)])
AC_ARG_VAR([master_top_srcdir],[path to the MPICH top-level source directory (if present)])

# Ensure that master_top_srcdir is set if maintainer mode for is set, 
# since some of the Makefile targets require it.
if test "X$master_top_srcdir" = "X" -a "$USE_MAINTAINER_MODE" = "yes" ; then
    if test -z "$top_srcdir" ; then
       use_top_srcdir=$srcdir
    else
       use_top_srcdir=$top_srcdir
    fi
    # Make use_top_srcdir absolute
    case "$use_top_srcdir" in
    /*) ;;
    *)
        use_top_srcdir=`(cd $use_top_srcdir && pwd)`
        ;;
    esac
fi

AC_ARG_ENABLE(fast,
	[AC_HELP_STRING([--enable-fast],
		[Indicates that the MPI implementation may have been
		built for fastest operation, such as building without
		error checking.])],,)

AC_ARG_ENABLE(xfail,
	[AC_HELP_STRING([--enable-xfail],
		[Run tests marked for expected failure])],,
	[enable_xfail=no])

AC_ARG_WITH(mpi,
	[AC_HELP_STRING([--with-mpi=dir],
		[Use the selected MPI; compilation scripts for mpicc,
		mpifort and mpicxx should be in dir/bin])],,)

# ------------------------------------------------------------------------
dnl use AC_ARG_VAR to mark FROM_MPICH as "precious" to autoconf so that
dnl automatic re-runs of config.status preserve its value correctly
AC_ARG_VAR([FROM_MPICH],[should be set to "yes" if this configure script is being invoked by the main MPICH configure])

#
# Only run xfail tests if enabled
RUN_XFAIL=false
if test "$enable_xfail" = "yes" ; then
    RUN_XFAIL=true
fi
AC_SUBST(RUN_XFAIL)

PAC_LOAD_BASE_CACHE
PAC_VPATH_CHECK()
PAC_PROG_MAKE

MPILIBLOC=""
AC_SUBST(MPILIBLOC)

# preserve these values across a reconfigure
AC_ARG_VAR([WRAPPER_CFLAGS],[])
AC_ARG_VAR([WRAPPER_CPPFLAGS],[])
AC_ARG_VAR([WRAPPER_LDFLAGS],[])
AC_ARG_VAR([WRAPPER_LIBS],[])

# Attach program prefix and suffix to executable names
PAC_GET_EXENAME("mpicc", MPICC_NAME)
PAC_GET_EXENAME("mpiexec", MPIEXEC_NAME)

if test "$FROM_MPICH" = "yes" ; then
    # perform configure tests with the normal compilers ($CC/etc), but use
    # the WRAPPER_xFLAGS computed by MPICH as our flags instead.  Then at the
    # end of configure we will empty out these flags and set our compilers to
    # the installed compiler wrappers
    CFLAGS="$WRAPPER_CFLAGS"
    CPPFLAGS="$WRAPPER_CPPFLAGS"
    LDFLAGS="$WRAPPER_LDFLAGS"

    # WRAPPER_LIBS contains currently non-existent libs like "-lopa" and "-lmpl"
    # right now, so set LIBS to the user-specified libs for now.
    # FIXME Does this need to be an AC_ARG_VAR?
    LIBS="$MPICH_LIBS"

elif test -n "$with_mpi" ; then
    if test -z "$MPICC" ; then
        CC=$with_mpi/bin/$MPICC_NAME
    else
        CC=$MPICC
    fi
    if test -z "$MPIEXEC" ; then
        MPIEXEC=$with_mpi/bin/$MPIEXEC_NAME
    fi
else
    # Try to use mpicc etc names
    if test -z "$MPICC" ; then
        AC_PATH_PROG(MPICC,$MPICC_NAME mpcc)
    fi
    if test "x$MPICC" != "x" ; then
        CC=$MPICC
    fi 
    if test -z "$MPIEXEC" ; then
        AC_PATH_PROG(MPIEXEC,$MPIEXEC_NAME)
    fi
fi

# Running C compiler tests
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

# Note that some versions of autoconf will insist that the compiler 
# produce executables at this point, which is why we must do something
# special for building within MPICH

# Ensure that we can compile an MPI program before we go any further
# We don't use a cached value here because this is a sanity check
# The exception is if we are executing this configure from within the 
# MPICH configure - in that case, the 
if test "$FROM_MPICH" != "yes" ; then
    AC_MSG_CHECKING([whether we can compile and link MPI programs in C])
    AC_LINK_IFELSE([
AC_LANG_PROGRAM([#include "mpi.h"],[MPI_Init(0,0);MPI_Finalize();])
    ],[mpi_compile_works=yes],[mpi_compile_works=no])
AC_MSG_RESULT($mpi_compile_works)

    if test "$mpi_compile_works" != "yes" ; then
       AC_MSG_ERROR([Unable to compile and/or link an MPI program!  Check config.log])
    fi
fi

# Check for const and restrict (used in some of the performance tests)
AC_C_CONST
AC_C_RESTRICT

# not using libtool for the test suite, so no LT_INIT.  Instead, test here
# for Library programs
AC_PROG_RANLIB
AM_PROG_AR

# Check for --enable-strict
PAC_ARG_STRICT

# General headers
AC_HEADER_STDC
dnl AC_CHECK_HEADERS(stdarg.h unistd.h string.h stdlib.h memory.h stdint.h)
dnl unistd.h string.h stdlib.h memory.h stdint.h are checked by AC_PROG_CC.
AC_CHECK_HEADERS(stdarg.h sys/time.h sys/resource.h)

PAC_ARG_THREAD_PACKAGE

THREAD_PACKAGE_NAME=THREAD_PACKAGE_INVALID
threadlib=""
case $with_thread_package in
    yes|posix|pthreads|solaris|uti)
        AC_CHECK_HEADERS(pthread.h)
        AC_CHECK_LIB([pthread],[pthread_key_create],[threadlib="-lpthread"])
        THREAD_PACKAGE_NAME=THREAD_PACKAGE_POSIX
        ;;
    win|windows)
        with_thread_package=win
        THREAD_PACKAGE_NAME=THREAD_PACKAGE_WIN
        AC_MSG_ERROR([The 'win' thread package is not supported via autoconf builds at this time.])
        ;;
    abt|argobots)
        with_thread_package=argobots
        PAC_CHECK_HEADER_LIB_FATAL([argobots], [abt.h], [abt], [ABT_key_create])
        threadlib="-labt"
        THREAD_PACKAGE_NAME=THREAD_PACKAGE_ARGOBOTS
        ;;
    no|none)
        with_thread_package=none
        THREAD_PACKAGE_NAME=THREAD_PACKAGE_NONE
        ;;
    *)
	AC_MSG_ERROR([The specified thread package, $with_thread_package, is not supported.])
        ;;
esac

# Define and export the selected thread library so that other packages
# know what's used
AC_DEFINE_UNQUOTED([THREAD_PACKAGE_NAME],[$THREAD_PACKAGE_NAME],[set to the name of the thread package])
AC_SUBST(threadlib)

# Check for h_addr or h_addr_list.  This is needed for the singjoin test
# in manual/singjoin.c
AC_CACHE_CHECK([whether struct hostent contains h_addr_list],
dnl Use Double quote LANG_PROGRAM
dnl so [] in h_addr_list[0] won't be ignored by IFELSE.
pac_cv_have_haddr_list,[
AC_COMPILE_IFELSE([
   AC_LANG_PROGRAM([#include <netdb.h>],[[
      struct hostent hp;
      hp.h_addr_list[0]=0;
   ]])
],[pac_cv_have_haddr_list=yes],[pac_cv_have_haddr_list=no])
])
if test "$pac_cv_have_haddr_list" = "yes" ; then
    AC_DEFINE(HAVE_H_ADDR_LIST,1,[Define if struct hostent contains h_addr_list])
fi

AC_CHECK_FUNCS(getrusage)

# Find perl; used to create some of the tests from template and 
# defintion files
AC_PATH_PROG(PERL,perl)
AC_SUBST(PERL)
AC_SUBST(otherlangs)
AC_SUBST(threadsdir)
AC_SUBST(MPIEXEC)
AC_SUBST(MAKE)
if test -z "$MPILIBNAME" ; then MPILIBNAME=mpich ; fi
AC_SUBST(MPILIBNAME)
dnl MPI_SOURCE may be set as an environment variable giving the location
dnl of the MPI implementation.  This is used only in runtests to include
dnl the location of the source of the MPI implementation into the XML
dnl summary file
AC_SUBST(MPI_SOURCE)

if test "$FROM_MPICH" = yes ; then
    # Set compilers/flags to be substituted in output files, e.g. Makefiles.
    LDFLAGS="$saveLDFLAGS"
    # note that the default definition of bindir is 
    #    '${exec_prefix}/bin'
    # so even if prefix is set, exec prefix is not until
    # the very last moment (i.e., not here). 
    if test "X$exec_prefix" = "XNONE" ; then
        saveExec_prefix=$exec_prefix
        if test "X$prefix" = "XNONE" ; then
            # Use the default
            exec_prefix=$ac_default_prefix
        else
            exec_prefix=$prefix
        fi
        # Evaluate with the current setting of exec_prefix
        eval mpibindir=${bindir}
        exec_prefix=$saveExec_prefix
    else
        eval mpibindir=${bindir}
    fi

    # we did our tests with the base compilers, now point the make system at the
    # installed compiler wrappers for actually building the tests
    CC=$mpibindir/$MPICC_NAME
    if test -z "$MPIEXEC" ; then
        MPIEXEC=$mpibindir/$MPIEXEC_NAME
    fi

    # Zero out the flags, since they are already contained in the compiler
    # wrapper scripts.  Note that this will kill any flags that have been added
    # to the xFLAGS only in this script.
    #
    # The only real flags we seem to add in this script relate to cray
    # pointer support in Fortran, so we include that var here where
    # appropriate.
    #
    # The other case are the performance tests - for datatype performance,
    # compiling with optimization is important.
    CFLAGS=""
    CPPFLAGS=""
    LDFLAGS=""
    LIBS=""
else
    # We need either mpiexec or mpirun.  If we don't find them,
    # the user will need to determine how to run a program
    AC_PATH_PROG(MPIEXEC,$MPIEXEC_NAME)
fi

dnl Note that this format for AC_OUTPUT can cause problems for autoconf
dnl run under cygwin
AC_OUTPUT( \
          Makefile \
          utest/Makefile \
          str/Makefile \
          )

