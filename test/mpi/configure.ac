##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

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
AC_INIT([mpich-testsuite],
        MPICH_VERSION_m4,
        [discuss@mpich.org],
        [mpich-testsuite],
        [http://www.mpich.org/])

AC_CONFIG_HEADERS([include/mpitestconf.h])
AH_TOP([/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
echo "RUNNING CONFIGURE FOR MPI TESTS"

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects no-dist])
AM_MAINTAINER_MODE([enable])

# Non-verbose make by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# save CXX before we overwrite it. We need it for NVCC's -ccbin
save_CXX="$CXX"

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

# these (in particular main_top_srcdir) are needed to regenerate
# the f90 files from the f77 files
AC_ARG_VAR([main_top_builddir],[path to the MPICH top-level build directory (if present)])
AC_ARG_VAR([main_top_srcdir],[path to the MPICH top-level source directory (if present)])

# Ensure that main_top_srcdir is set if maintainer mode for is set,
# since some of the Makefile targets require it.
if test "X$main_top_srcdir" = "X" -a "$USE_MAINTAINER_MODE" = "yes" ; then
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
    # Now, see if we can find the f77tof90 routine
    if test -x $use_top_srcdir/maint/f77tof90 ; then
        main_top_srcdir=$use_top_srcdir
    else
        AC_MSG_ERROR([Unable to find main source file - reconfigure using --disable-maintainer_mode])
    fi
fi

AC_ARG_ENABLE(echo,
        [AC_HELP_STRING([--enable-echo],[Turn on strong echoing. The default is enable=no.])],
        [set -x])

AC_ARG_ENABLE(fortran,
[  --enable-fortran=option - Control the level of Fortran support in the MPICH implementation.
        yes|all   - Enable all available Fortran implementations (F77, F90, F08)
        no|none   - No Fortran support
],,[enable_fortran=all])

save_IFS="$IFS"
IFS=","
enable_f77=no
enable_fc=no
enable_f08=no
for option in $enable_fortran ; do
    case "$option" in
        yes|all)
                enable_f77=yes
                enable_fc=yes
                enable_f08=yes
                ;;
        no|none)
                enable_f77=no
                enable_fc=no
                enable_f08=no
                ;;
        *)
                IFS="$save_IFS"
                AC_MSG_WARN([Unknown value $option for --enable-fortran])
                IFS=","
                ;;
    esac
done
IFS="$save_IFS"

# DTPools switch
AC_ARG_ENABLE([dtpools],
              [AS_HELP_STRING([--disable-dtpools],[Disable dtpools tests])],,
              [enable_dtpools=yes])

# runtests options
RUNTESTS_OPTS=
AC_SUBST(RUNTESTS_OPTS)

AC_ARG_ENABLE(cxx,
        [AC_HELP_STRING([--enable-cxx],[Turn on C++ tests (default)])],,[enable_cxx=yes])

AC_ARG_ENABLE(romio,
        [AC_HELP_STRING([--enable-romio],[Enable MPI I/O tests])])

AC_ARG_ENABLE(namepub,
        [AC_HELP_STRING([--enable-namepub],
                [Enable tests of the dynamic process parts of MPI-2 (default)])],,
        [enable_namepub=yes])

AC_ARG_ENABLE(spawn,
        [AC_HELP_STRING([--enable-spawn],
                [Enable tests of the dynamic process parts of MPI-2 (default)])],,
        [enable_spawn=yes])

AC_ARG_ENABLE(rma,
        [AC_HELP_STRING([--enable-rma],[Enable tests of the one sided parts of MPI-2 (default)])],,
        [enable_rma=yes])

AC_ARG_ENABLE(part,
        [AS_HELP_STRING([--enable-part],[Enable tests of partitioned communication of MPI-4 (default)])],,
        [enable_part=yes])

AC_ARG_ENABLE(threadcomm,
        [AS_HELP_STRING([--enable-threadcomm],[Enable tests of MPIX_Threadcomm (default)])],,
        [enable_threadcomm=yes])

AC_ARG_ENABLE(long-double-complex,
        [AS_HELP_STRING([--enable-long-double-complex],
                [Enable tests involving MPI_LONG_DOUBLE_COMPLEX (default)])],,
        [enable_long_double_complex=yes])
AC_ARG_ENABLE(long-double,
        [AS_HELP_STRING([--enable-long-double-complex],
                [Enable tests involving MPI_LONG_DOUBLE and related types (default)])],,
        [enable_long_double=yes])

AC_ARG_ENABLE(checkerrors,
        [AS_HELP_STRING([--enable-checkerrors],
                [Add some tests for checking for errors in user programs])],,
        [enable_checkerrors=yes])

AC_ARG_ENABLE(perftest,
        [AS_HELP_STRING([--enable-perftest],
                [Include tests for basic performance consistency (default)])],,
        [enable_perftest=yes])

AC_ARG_ENABLE(large-tests,
        [AS_HELP_STRING([--enable-large-tests],
                [Enable tests which require large (>2GB per proc) amount of memory])],,
        [enable_large_tests=no])

AC_ARG_ENABLE(ft-tests,
        [AS_HELP_STRING([--enable-ft-tests],
                [Include tests for fault tolerance (default no)])],,
        [enable_ft_tests=no])

AC_ARG_ENABLE(comm-overlap-tests,
        [AS_HELP_STRING([--enable-comm-overlap-tests],
                [Include tests for communicator overlap (default)])],,
        [enable_comm_overlap_tests=yes])

AC_ARG_ENABLE(checkfaults,
        [AS_HELP_STRING([--enable-checkfaults],
                [Add some tests for checking on handling of faults in user programs])],,
        [enable_checkfaults=no])

AC_ARG_ENABLE(checkpointing,
        [AS_HELP_STRING([--enable-checkpointing],
                [Add some tests for checkpointing])],,
        [enable_checkpointing=no])

AC_ARG_ENABLE(fast,
        [AS_HELP_STRING([--enable-fast],
                [Indicates that the MPI implementation may have been
                built for fastest operation, such as building without
                error checking.  Has the effect of
                --enable-checkerrors=no])],,)

AC_ARG_ENABLE(strictmpi,
        [AS_HELP_STRING([--enable-strictmpi],
                [Only test for operations specifically defined by the
                MPI standard.  This turns off tests for some common
                extensions, including for combinations of predefined
                datatypes and predefined MPI_Op s.])],,
        [enable_strictmpi=no])

AC_ARG_ENABLE(threads,
        [AS_HELP_STRING([--enable-threads],
                [Whether to enable multi-thread tests])],,
        [enable_threads=yes])

AC_ARG_ENABLE(xfail,
        [AC_HELP_STRING([--enable-xfail],
                [Run tests marked for expected failure])],,
        [enable_xfail=no])

AC_ARG_ENABLE(collalgo-tests,
        [AS_HELP_STRING([--enable-collalgo-tests],
                [Run extended collective algorithm tests])],,
        [enable_collalgo_tests=yes])

AC_ARG_ENABLE(gpu-tests-only,
        [AS_HELP_STRING([--enable-gpu-tests-only],
                [Run only GPU tests])],,
        [enable_gpu_tests_only=no])

# DTPools test generation
AC_ARG_WITH(dtpools-datatypes,
    [AS_HELP_STRING([--with-dtpools-datatypes=typelist],
        [Comma separated list of MPI datatypes to use for DTPools tests
         generation. Typelist can be of the form: 'MPI_INT,MPI_DOUBLE,...',
         for single element types, 'MPI_INT:4+MPI_DOUBLE:8,...', for multiple
         element types, or a combination of both.])],,[with_dtpools_datatypes=MPI_INT,MPI_INT:4+MPI_DOUBLE:8])

# parse args for typegen.sh script
dtp_args="--with-dtpools-datatypes=${with_dtpools_datatypes}"
AC_CONFIG_COMMANDS([gentests],[$srcdir/maint/gentests_dtp.sh $dtp_args],[dtp_args="$dtp_args"])

AC_ARG_WITH(mpi,
        [AC_HELP_STRING([--with-mpi=dir],
                [Use the selected MPI; compilation scripts for mpicc,
                mpifort and mpicxx should be in dir/bin])],,)

AC_ARG_WITH(pm,
        AC_HELP_STRING([--with-pm=name],
                [Specify the process manager for MPICH.  "no" or "none" are
                 valid values.  Multiple process managers may be specified as
                 long as they all use the same pmi interface by separating them
                 with colons.  The mpiexec for the first named process manager
                 will be installed.  Example: "--with-pm=hydra:gforker"
                 builds the two process managers hydra and gforker;
                 only the mpiexec from hydra is installed into the bin
                 directory.]),,with_pm=default)

if test "$with_pm" = "none" ; then
    # add "none" as synonym for "no" to agree with older erroneous docs
    with_pm="no"
fi
if test "$MPID_NO_PM" = yes ; then
    if test "$with_pm" != "default" -a "$with_pm" != no ; then
        AC_MSG_ERROR([The PM chosen ($with_pm) is is not valid for the selected device ($with_device)])
    fi
    # This is used to change with_pm=default to with_pm=no in the case
    # where the device does not want a PM
    with_pm=no
fi
if test -z "$with_pm" ; then
    with_pm="no"
fi
if test "$with_pmi" = "uni" -a "$with_pm" = "default" ; then
    with_pm="no"
fi
if test "$with_pm" = "default" -o "$with_pm" = "yes" ; then
   if test ! -z "$MPID_DEFAULT_PM" ; then
      with_pm=${MPID_DEFAULT_PM}
   else
      with_pm=hydra
   fi
fi

# Get the first pm specified
if test "$with_pm" != "no" ; then
    first_pm="`echo $with_pm | sed -e 's/:.*//' -e 's/,.*//'`"
else
    first_pm=""
fi

AC_ARG_WITH(config-args,
        [AC_HELP_STRING([--with-config-args=filename],
                [Specify configure argument file that contains the
                values of variables that configure reads,
                e.g. CC,CFLAGS,F77,FFLAGS,FC,FCFLAGS....  If the
                filename does not begin with / (absolute path), . or
                .. (relative path), the filename will be assumed to be
                $top_srcdir/configargs/<filename>.cfg.])],,
        [with_config_args=no])

if test "$with_config_args" != "no" ; then
    case "$with_config_args" in
        /*|../*|./*)
            config_args_file="$with_config_args"
            ;;
        *)
            config_args_file="$srcdir/configargs/$with_config_args.cfg"
            ;;
    esac
    if test -s "$config_args_file" ; then
        AC_MSG_RESULT([Reading the configure arguments in ${config_args_file}.])
        . $config_args_file
        # Export all the variables in $config_args_file
        # i.e. CC,CFLAGS, F77/FFLAGS, FC/FCFLAGS, CXX/CXXFLAGS and friends...
        config_args_vars=`grep -v '^#' $config_args_file | sed -e 's/=.*//g'`
        for var in $config_args_vars ; do
            eval value=\$"$var"
            echo "Exporting $var=$value ..."
            export $var
        done
    else
        AC_MSG_WARN([Non-existent ${config_args_file}.])
    fi
fi

# Attach program prefix and suffix to executable names
PAC_GET_EXENAME("mpicc", MPICC_NAME)
PAC_GET_EXENAME("mpif77", MPIF77_NAME)
PAC_GET_EXENAME("mpifort", MPIFORT_NAME)
PAC_GET_EXENAME("mpicxx", MPICXX_NAME)
PAC_GET_EXENAME("mpiexec", MPIEXEC_NAME)

if test -n "$with_mpi" ; then
    if test -z "$MPICC" ; then
        CC=$with_mpi/bin/$MPICC_NAME
    else
        CC=$MPICC
    fi
    if test -z "$MPIF77" ; then
        F77=$with_mpi/bin/$MPIF77_NAME
    else
        F77=$MPIF77
    fi
    if test -z "$MPIFC" ; then
        FC=$with_mpi/bin/$MPIFORT_NAME
    else
        FC=$MPIFC
    fi
    if test -z "$MPICXX" ; then
        CXX=$with_mpi/bin/$MPICXX_NAME
    else
        CXX=$MPICXX
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
    if test -z "$MPIF77" ; then
        AC_PATH_PROG(MPIF77,$MPIF77_NAME mpf77)
    fi
    if test "x$MPIF77" != "x" ; then
        F77=$MPIF77
    fi
    if test -z "$MPIFC" ; then
        AC_PATH_PROG(MPIFC,$MPIFORT_NAME mpftn)
    fi
    if test "x$MPIFC" != "x" ; then
        FC=$MPIFC
    fi
    if test -z "$MPICXX" ; then
        # We left mpiCC off of this list because mpicc and mpiCC are the
        # same on Mac OSX systems.
        AC_PATH_PROG(MPICXX,$MPICXX_NAME mpCC)
    fi
    if test "x$MPICXX" != "x" ; then
        CXX=$MPICXX
    fi
    if test -z "$MPIEXEC" ; then
        AC_PATH_PROG(MPIEXEC,$MPIEXEC_NAME mpiexec)
    fi
fi
dnl MPIEXEC is used in the Makefile testing target.
AC_SUBST([MPIEXEC])

# Make sure we export CC before configuring DTPools, since MPI is
# needed to build the library.
export CC
PAC_CONFIG_SUBDIR_ARGS([dtpools])

# Running C compiler tests
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_USE_SYSTEM_EXTENSIONS

# Ensure that we can compile an MPI program before we go any further
# We don't use a cached value here because this is a sanity check
AC_MSG_CHECKING([whether we can compile and link MPI programs in C])
AC_LINK_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[MPI_Init(0,0);MPI_Finalize();])
    ],[mpi_compile_works=yes],[mpi_compile_works=no])
AC_MSG_RESULT($mpi_compile_works)

if test "$mpi_compile_works" != "yes" ; then
    AC_MSG_ERROR([Unable to compile and/or link an MPI program!  Check config.log])
fi

# Check whether we are testing MPI derived from MPICH
MPI_IS_MPICH=no
AC_MSG_CHECKING([Is the MPI derived from MPICH])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[return 1 + MPICH;])],
    [MPI_IS_MPICH=yes],[MPI_IS_MPICH=no])
AC_MSG_RESULT($MPI_IS_MPICH)

if test "$MPI_IS_MPICH" = "yes" ; then
    # try mpichversion
    AC_PATH_PROG(MPICHVERSION, mpichversion, [], [$PATH:$with_mpi/bin])
    if test -n "$MPICHVERSION" ; then
        $MPICHVERSION > conftest.output
        # netmod
        if [grep 'MPICH Device:[[:blank:]]*ch4:ofi' conftest.output > /dev/null] ; then
            AC_MSG_NOTICE([Detected MPICH with ch4:ofi netmod])
            AC_DEFINE(MPICH_CH4_OFI, 1, [define if it is mpich with ch4:ofi])
        elif [grep 'MPICH Device:[[:blank:]]*ch4:ucx' conftest.output > /dev/null] ; then
            AC_MSG_NOTICE([Detected MPICH with ch4:ucx netmod])
            AC_DEFINE(MPICH_CH4_UCX, 1, [define if it is mpich with ch4:ucx])
            enable_spawn=no
        elif [grep 'MPICH Device:[[:blank:]]*ch3' conftest.output > /dev/null] ; then
            enable_part=no
        fi

        if ! grep 'MPICH features:.*threadcomm' conftest.output > /dev/null ; then
            enable_threadcomm=no
        fi

        # enable_checkerrors
        if test "$enable_checkerrors" != "no" ; then
            if grep 'disable-error-checking\|enable-error-checking=no' conftest.output > /dev/null ; then
                enable_checkerrors=no
            fi
        fi

        #enable_spawn
        if test "$enable_spawn" != "no" ; then
            if grep 'enable-ch4-am-only' conftest.output > /dev/null ; then
                enable_spawn=no
            fi
        fi

        rm conftest.output
    fi
fi

AM_CONDITIONAL([NOT_STRICTMPI], [test x$enable_strictmpi = xno])
if test "$enable_strictmpi" = "yes" ; then
    AC_DEFINE([ENABLE_STRICTMPI], [1], [Define if configured with --enable-strictmpi])
fi

if test "$enable_threadcomm" != "no" ; then
    AC_DEFINE(HAVE_THREADCOMM, 1, [Define if MPIX_Threadcomm is available])
fi

# First, determine whether we are/can support the language bindings
#
# Since F90/F90FLAGS are replaced by FC/FCFLAGS, rather than silently
# substituting them, i.e. FC=$F90 and FCFLAGS=$F90FLAGS, we choose to emit
# an error message and abort to avoid any ambiguous/hidden bug in choosing
# Fortran90 compilers.
if test -n "$F90" -o -n "$F90FLAGS" ; then
    AC_MSG_ERROR([F90 and F90FLAGS are replaced by FC and FCFLAGS respectively in this configure, please unset F90/F90FLAGS and set FC/FCFLAGS instead and rerun configure again.])
fi

# errordir is substituted into the testlist file as errors when the
# tests should check error handling and as a comment (#) otherwise.
errordir="#"
if test "$enable_checkerrors" = "yes" ; then
    errordir=errors
fi
AC_SUBST(errordir)

# The performance tests are not part of the MPI standard
perfdir="perf"
if test "$enable_strictmpi" = "yes" -o "$enable_perftest" = "no" ; then
     perfdir="#"
fi
AC_SUBST(perfdir)

# The ft tests are not part of the MPI standard and some of the netmods can't handle them
ftdir="#ft"
if test "$first_pm" = "hydra" -a  "$enable_strictmpi" = "no" -a "$enable_ft_tests" = "yes" ; then
    ftdir="ft"
fi
AC_SUBST(ftdir)

# Setup "comm_overlap" variable based on whether comm_overlap tests
# are enabled or not
if test "${enable_comm_overlap_tests}" = "yes" ; then
   comm_overlap=""
else
   comm_overlap="#"
fi
AC_SUBST(comm_overlap)

# The multi-thread tests can be disabled by --disable-threads
threadsdir="threads"
if test "$enable_threads" != "no" ; then
    AC_MSG_CHECKING([whether MPI_THREAD_MULTIPLE is supported])
    AC_RUN_IFELSE([
            AC_LANG_PROGRAM([#include "mpi.h"],[
                int provided;
                MPI_Init_thread(0,0, MPI_THREAD_MULTIPLE, &provided);
                MPI_Finalize();
                if (provided != MPI_THREAD_MULTIPLE) {
                    return 1;
                }
            ])
        ],[enable_threads=yes],[enable_threads=no])
    AC_MSG_RESULT($enable_threads)
fi

if test "$enable_threads" = "no"; then
    threadsdir=""
fi
#
# Only run the checkpointing tests if enabled
ckpointdir="#ckpoint"
if test "$enable_checkpointing" = "yes" ; then
    ckpointdir="ckpoint"
fi
AC_SUBST(ckpointdir)

#
# Only run xfail tests if enabled
if test "$enable_xfail" = "yes" ; then
    RUNTESTS_OPTS="$RUNTESTS_OPTS -runxfail"
fi

PAC_LOAD_BASE_CACHE
PAC_VPATH_CHECK()
PAC_PROG_MAKE

MPILIBLOC=""
AC_SUBST(MPILIBLOC)

namepub_tests="#"
if test "$enable_namepub" = "yes" ; then
    namepub_tests=""
fi
AC_SUBST(namepub_tests)

# Some MPI-2 implementations (including some of the MPICH shared-memory
# channels and BG/L) leave out the dynamic process routines.  This
# allows tests to avoid reporting failure for these routines.
# This can be controlled by either a --disable-spawn argument or by
# setting the environment variable MPI_NO_SPAWN to yes.
AC_ARG_VAR([MPI_NO_SPAWN],[set to "yes" to disable dynamic process tests])
if test "$enable_spawn" = "yes" -a "$MPI_NO_SPAWN" != "yes" ; then
    spawndir=spawn
    AC_DEFINE(HAVE_MPI_SPAWN,1,[Define if Dynamic Process functionality is available])
fi
AC_SUBST(spawndir)

# Also allow rma to be disabled
AC_ARG_VAR([MPI_NO_RMA],[set to "yes" to disable one-sided tests])
rmadir=rma
if test "$enable_rma" != yes ; then
    rmadir="#"
elif test "$MPI_NO_RMA"  = yes ; then
    rmadir="#"
else
    AC_DEFINE(HAVE_MPI_WIN_CREATE,1,[Define if MPI_Win_create is available])
fi
AC_SUBST(rmadir)

# Also allow partitioned communication to be disabled
AC_ARG_VAR([MPI_NO_PART],[set to "yes" to disable partitioned communication tests])
partdir=part
if test "$enable_part" != "yes" -o "$MPI_NO_PART" = "yes" ; then
    partdir="#"
fi
AC_SUBST(partdir)
threadcommdir=threadcomm
if test "$enable_threadcomm" != "yes" ; then
    threadcommdir="#"
fi
AC_SUBST(threadcommdir)

faultsdir=#
if test "$enable_checkfaults" = "yes" ; then
    faultsdir=faults
fi
AC_SUBST(faultsdir)
#
if test "$enable_strictmpi" = "yes" ; then
    RUNTESTS_OPTS="$RUNTESTS_OPTS -strict"
    AC_DEFINE(USE_STRICT_MPI,1,[Define if only operations defined in MPI should be tested])
fi
#
# At this writing, MPICH has many MPIX routines, and the test suite includes
# them.  As these are not MPI routines (yet), they are invalid and incorrect
# when this test suite is used for other MPI implementations, including those
# based on earlier versions of MPICH.
MPI_HAS_MPIX=no
#
# If MPI is derived from MPICH and strictmpi not selected, let's test mpix extensions.
if test "$MPI_IS_MPICH" = "yes" -a "$enable_strictmpi" = "no" ; then
   MPI_HAS_MPIX=yes
fi
AC_SUBST(MPI_HAS_MPIX)

# Prepend @mpix@ to lines of tests in testlist.in which are MPIX tests so that
# we can skip running these tests when we do strict MPI test.
mpix="#"
if test "$enable_strictmpi" = "no"; then
    mpix=""
fi
AC_SUBST(mpix)

# Use the conditional variable BUILD_MPIX_TESTS to conditionally add MPIX tests
# to noninst_PROGRAMS to skip building the tests when we do strict MPI test
AM_CONDITIONAL([BUILD_MPIX_TESTS], [test "$enable_strictmpi" = "no"])

dnl We cannot use AC_C_LONG_DOUBLE
dnl because it does not support cross-compilation.  Instead, we use the
dnl same test in the MPICH configure.
# Check on support for long double and long long types.  Do this before the
# structure alignment test because it will test for including those
# types as well
#
# If --disable-long-double is selected, then bypass this test.
# Some MPI implementations may choose to not support long double because
# their C compilers are inconsistent on the length of long double (this
# is the case on the Cray, with Cray, PGI, and GNU not agreeing on the
# size of long double)
if test "$enable_long_double" = "yes" ; then
    AC_CACHE_CHECK([whether long double is supported],pac_cv_have_long_double,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([],[long double a;])
    ],[pac_cv_have_long_double=yes],[pac_cv_have_long_double=no])
])
    if test "$pac_cv_have_long_double" = "yes" ; then
        AC_DEFINE(HAVE_LONG_DOUBLE,1,[Define if long double is supported])
    fi
fi
AC_CACHE_CHECK([whether long long is supported],pac_cv_have_long_long,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([],[long long a;])
    ],[pac_cv_have_long_long=yes],[pac_cv_have_long_long=no])
])
if test "$pac_cv_have_long_long" = yes ; then
    AC_DEFINE(HAVE_LONG_LONG,1,[Define if compiler supports long long])
fi
#
# Check for const and restrict (used in some of the performance tests)
AC_C_CONST
AC_C_RESTRICT


# Archiver for building utility libraries
AM_PROG_AR

# Check for --enable-strict
# NOTE: do this before checking any AC_CHECK_HEADERS, because strict may
# turn on extension macros
PAC_ARG_STRICT

# General headers
dnl AC_CHECK_HEADERS(stdarg.h unistd.h string.h stdlib.h memory.h stdint.h)
dnl unistd.h string.h stdlib.h memory.h stdint.h are checked by AC_PROG_CC.
AC_CHECK_HEADERS(stdarg.h sys/time.h sys/resource.h)

# check if we need declarations
AC_CHECK_FUNCS(strdup)
if test "$ac_cv_func_strdup" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
fi

AC_CHECK_FUNCS(usleep)
if test "$ac_cv_func_usleep" = "yes" ; then
   PAC_FUNC_NEEDS_DECL([#include <unistd.h>],usleep)
fi

# Check for fixed width types
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Check for availability of C99 types
AC_CHECK_TYPES([_Bool, float _Complex, double _Complex, long double _Complex])
AC_CHECK_SIZEOF(void *,8)

# Start of GPU libs check
have_gpu="no"
PAC_PUSH_FLAG([CPPFLAGS])
PAC_PUSH_FLAG([LDFLAGS])
PAC_PUSH_FLAG([LIBS])

# Check CUDA availability
PAC_CHECK_HEADER_LIB_OPTIONAL([cuda],[cuda_runtime_api.h],[cudart],[cudaStreamSynchronize])
cuda_CPPFLAGS=""
cuda_LDFLAGS=""
cuda_LIBS=""
cudadir=
AC_SUBST(cudadir)
AC_ARG_VAR([NVCC], [nvcc compiler to use])
if test "X${pac_have_cuda}" = "Xyes" ; then
    AC_DEFINE([HAVE_CUDA],[1],[Define if CUDA is available])
    have_gpu="yes"
    if test -n "${with_cuda}" -a "$with_cuda" != "no" ; then
        cuda_CPPFLAGS="-I${with_cuda}/include"
        if test -d ${with_cuda}/lib64 ; then
            cuda_LDFLAGS="-L${with_cuda}/lib64 -L${with_cuda}/lib"
        else
            cuda_LDFLAGS="-L${with_cuda}/lib"
        fi
    fi

    if test -z "$NVCC" ; then
        if test -n "${with_cuda}" -a "$with_cuda" != "no" ; then
            AC_PATH_PROG([NVCC], [nvcc], [nvcc_not_found], [$with_cuda/bin:$PATH])
        else
            AC_PATH_PROG([NVCC], [nvcc], [nvcc_not_found])
        fi
        if test "$NVCC" != "nvcc_not_found" -a -n "$save_CXX" ; then
            NVCC="$NVCC -ccbin $save_CXX"
        fi
    fi
    cuda_LIBS="-lcudart"

    cudadir="cuda"
fi
AM_CONDITIONAL([HAVE_CUDA],[test "X${pac_have_cuda}" = "Xyes"])
AC_SUBST([cuda_CPPFLAGS])
AC_SUBST([cuda_LDFLAGS])
AC_SUBST([cuda_LIBS])

if test "$have_gpu" = "no" ; then
    # Check Level Zero availability when no other GPU library is available
    PAC_CHECK_HEADER_LIB_OPTIONAL([ze],[level_zero/ze_api.h],[ze_loader],[zeInit])
    ze_CPPFLAGS=""
    ze_LDFLAGS=""
    ze_LIBS=""
    if test "X${pac_have_ze}" = "Xyes" ; then
        AC_DEFINE([HAVE_ZE],[1],[Define if ZE is available])
        have_gpu="yes"
        if test -n "${with_ze}" -a "$with_ze" != "no" ; then
            ze_CPPFLAGS="-I${with_ze}/include"
            if test -d ${with_ze}/lib64 ; then
                ze_LDFLAGS="-L${with_ze}/lib64 -L${with_ze}/lib"
            else
                ze_LDFLAGS="-L${with_ze}/lib"
            fi
        fi
        ze_LIBS="-lze_loader"
    fi
    AC_SUBST([ze_CPPFLAGS])
    AC_SUBST([ze_LDFLAGS])
    AC_SUBST([ze_LIBS])
fi
AM_CONDITIONAL([HAVE_ZE],[test "X${pac_have_ze}" = "Xyes"])

# Check HIP availability
hipdir=
AC_SUBST(hipdir)
AC_ARG_VAR([HIPCC], [hipcc compiler to use])
if test "$have_gpu" = "no" ; then
    AC_DEFINE([__HIP_PLATFORM_AMD__],[1],[AMD GPU HIP available])
    PAC_CHECK_HEADER_LIB_OPTIONAL([hip],[hip/hip_runtime_api.h],[amdhip64],[hipStreamSynchronize])
    hip_CPPFLAGS=""
    hip_LDFLAGS=""
    hip_LIBS=""
    if test "X$pac_have_hip" = "Xyes" ; then
       AC_DEFINE([HAVE_HIP],[1],[Define if HIP is available])
       have_gpu="yes"
       if test -n "${with_hip}" -a "$with_hip" != "no" ; then
          hip_CPPFLAGS="-I${with_hip}/include"
          if test -d ${with_hip}/lib64 ; then
              hip_LDFLAGS="-L${with_hip}/lib64 -L${with_hip}/lib"
          else
              hip_LDFLAGS="-L${with_hip}/lib"
          fi
       fi
       hip_LIBS="-lamdhip64"

       if test -z "$HIPCC" ; then
           if test -n "${with_hip}" -a "$with_hip" != "no" ; then
               AC_PATH_PROG([HIPCC], [hipcc], [hipcc_not_found], [$with_hip/bin:$PATH])
           else
               AC_PATH_PROG([HIPCC], [hipcc], [hipcc_not_found])
           fi
       fi

       hipdir="hip"
    fi
    AC_SUBST([hip_CPPFLAGS])
    AC_SUBST([hip_LDFLAGS])
    AC_SUBST([hip_LIBS])
fi
AM_CONDITIONAL([HAVE_HIP],[test "X${pac_have_hip}" = "Xyes"])

PAC_POP_FLAG([CPPFLAGS])
PAC_POP_FLAG([LDFLAGS])
PAC_POP_FLAG([LIBS])

# setup which tests "make testing" will run
default_testlist=testlist
default_testdirs=
if test $have_gpu = "yes" ; then
    default_testlist="${default_testlist},testlist.gpu"
fi
if test $enable_dtpools = "yes" ; then
    default_testlist="${default_testlist},testlist.dtp"
fi
if test $enable_collalgo_tests = "yes" ; then
    default_testlist="${default_testlist},testlist.collalgo"
fi
# handle --enable-gpu-tests-only
if test $have_gpu = "no" -a $enable_gpu_tests_only = "yes" ; then
    AC_MSG_ERROR([GPU only test configuration requires GPU library (CUDA or LevelZero)])
elif test $enable_gpu_tests_only = "yes" ; then
    default_testdirs=coll,pt2pt,rma
    default_testlist=testlist.gpu
fi
AC_SUBST([default_testdirs])
AC_SUBST([default_testlist])

# for tests that require large mem
largetest="#"
if test "$enable_large_tests" = "yes" -a $ac_cv_sizeof_void_p -ge 8; then
    largetest=""
fi
AC_SUBST(largetest)

# Enable device specific tests in impls/mpich
ch3_tests="#"
ch4_tests="#"
case $with_device in
    ch3*)
        ch3_tests=""
        ;;
    ch4*)
        ch4_tests=""
        ;;
esac
ch4_ucx_tests="#"
ch4_ofi_tests="#"
case $with_device in
    ch4:ucx*)
        ch4_ucx_tests=""
        ;;
    ch4:ofi*)
        ch4_ofi_tests=""
        ;;
esac
AC_SUBST(ch3_tests)
AC_SUBST(ch4_tests)
AC_SUBST(ch4_ucx_tests)
AC_SUBST(ch4_ofi_tests)

# Only run the long double complex tests if that type is available
if test "x$enable_long_double" = "xyes" && \
   test "x$enable_long_double_complex" = "xyes" && \
   test "x$ac_cv_type_long_double__Complex" = "xyes" ; then
   AC_DEFINE(USE_LONG_DOUBLE_COMPLEX,1,[Define if tests with long double complex should be included])
fi

# extra libraries may be necessary on some platforms (solaris) for spawn/join
if test "$spawndir" = "spawn" ; then
    PAC_PUSH_FLAG(LIBS)
    AC_SEARCH_LIBS(socket,socket,socklib=$LIBS)
    PAC_POP_FLAG(LIBS)
    PAC_PUSH_FLAG(LIBS)
    AC_SEARCH_LIBS(gethostbyname,nsl,nslib=$LIBS)
    PAC_POP_FLAG(LIBS)
    AC_SUBST(socklib)
    AC_SUBST(nslib)
fi

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

# Check for the MPI Version.  This test assumes at least MPI 2.0.  For
# some tests, we need to know if we are MPI 2.1 or MPI 2.2,
# particularly for new routines in Fortran
AC_CACHE_CHECK([for major version of MPI],pac_cv_mpi_major_version,[
    for pac_cv_mpi_major_version in 4 3 2 1 unknown ; do
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[
#if MPI_VERSION == $pac_cv_mpi_major_version
    ''' force failure '''
#endif])],,break)
    done
])

AC_CACHE_CHECK([for minor version of MPI],pac_cv_mpi_minor_version,[
    for pac_cv_mpi_minor_version in 4 3 2 1 0 unknown ; do
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[
#if MPI_SUBVERSION == $pac_cv_mpi_minor_version
    ''' force failure '''
#endif])],,break)
    done
])
MPI_VERSION=$pac_cv_mpi_major_version
MPI_SUBVERSION=$pac_cv_mpi_minor_version

AC_SUBST(MPI_VERSION)
AC_SUBST(MPI_SUBVERSION)

AC_MSG_CHECKING([whether MPI-IO is available])
AC_LINK_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_File fh;
            MPI_Init(0,0);
            MPI_File_open(MPI_COMM_WORLD,"/dev/null",MPI_MODE_WRONLY,MPI_INFO_NULL,&fh);
            MPI_File_close(&fh);
            MPI_Finalize();])
    ],[mpi_io_works=yes],[mpi_io_works=no])
AC_MSG_RESULT($mpi_io_works)

# Running Fortran 77 compiler tests
AC_PROG_F77
if test "$enable_f77" = yes ; then
    # If there is no working F77, then set enable_f77 to no
    if test -z "$F77" ; then
        enable_f77=no
    fi
fi
# Simple tests for which other languages we can handle.
# Use these only when configuring separate from an MPICH build
f77dir="#"
AC_SUBST(f77dir)
buildingF77=no
if test "$enable_f77" = yes ; then
    AC_MSG_CHECKING([that we can build MPI programs with Fortran 77])
    AC_LANG_PUSH([Fortran 77])
    AC_LINK_IFELSE([
        AC_LANG_SOURCE([
            program main
            include 'mpif.h'
            integer ierr
            call mpi_init(ierr)
            call mpi_finalize(ierr)
            end
        ])
    ],[
        AC_MSG_RESULT(yes)
        otherlangs="$otherlangs f77"
        f77dir=f77
        buildingF77=yes
    ],[
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([Fortran 77])
fi
#
# At least one test (C++ test of C and Fortran datatypes) needs to
# know if Fortran is supported
if test "$f77dir" = "f77" ; then
    AC_DEFINE(HAVE_FORTRAN_BINDING,1,[Define if Fortran is supported])
fi


AC_ARG_VAR([MPI_SIZEOF_AINT],[if set, force MPI_Aint to a width of this many bytes])
AC_ARG_VAR([MPI_SIZEOF_OFFSET],[if set, force MPI_Offset to a width of this many bytes])

# Common tests for F77
if test "$buildingF77" = "yes" ; then
    # Match integer types to the MPI types for MPI_Aint and MPI_Offset

    # FIXME: Add a test to see if the environment is importing the
    # necessary information.

    # Get the sizes of the MPI types.  We use the following:
    # MPI_SIZEOF_OFFSET and MPI_SIZEOF_AINT
    if test -z "$MPI_SIZEOF_AINT" ; then
        # Aint should be an address-sized integer, the same as void*
        # We use a test on the size of void * to avoid any complications
        # in dealing with running programs containing MPI headers (e.g.,
        # the IBM MPI changes the main entry point so that MPI
        # programs cannot be run on the host node)
        AC_CHECK_SIZEOF(void *)
        MPI_SIZEOF_AINT=$ac_cv_sizeof_void_p
    fi
    if test -z "$MPI_SIZEOF_OFFSET" ; then
        AC_CHECK_SIZEOF([MPI_Offset],[],[#include "mpi.h"])
        if test "$ac_cv_sizeof_MPI_Offset" = "unknown" \
             -o "$ac_cv_sizeof_MPI_Offset" -eq 0 ; then
             AC_MSG_WARN([Using 8 for the size of MPI_Offset])
             MPI_SIZEOF_OFFSET=8
        else
             MPI_SIZEOF_OFFSET=$ac_cv_sizeof_MPI_Offset
        fi
    fi

    AC_LANG_PUSH([Fortran 77])
    AC_CACHE_CHECK([whether integer*4 is supported],pac_cv_fort_integer4,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      integer*4 i])],
        pac_cv_fort_integer4=yes,
        pac_cv_fort_integer4=no)])
    AC_CACHE_CHECK([whether integer*8 is supported],pac_cv_fort_integer8,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      integer*8 i])],
        pac_cv_fort_integer8=yes,
        pac_cv_fort_integer8=no)])
    AC_CACHE_CHECK([whether integer*16 is supported],pac_cv_fort_integer16,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      integer*16 i])],
        pac_cv_fort_integer16=yes,
        pac_cv_fort_integer16=no)])
    AC_LANG_POP([Fortran 77])

    # Determine Aint and Offset
    for len in 4 8 16 ; do
        eval testval=\$"pac_cv_fort_integer$len"
        if test "$testval" = no ; then continue ; fi
        testval=$len
        if test $len = "$MPI_SIZEOF_AINT" ; then
            F77_MPI_ADDRESS="integer*$len"
        fi
        if test $len = "$MPI_SIZEOF_OFFSET" ; then
            F77_MPI_OFFSET="integer*$len"
        fi
    done
    # At this point, we could create a test program that would confirm that
    # the values in Fortran matched the values in C.
    # Note that we must make this an MPI program because the compiler for
    # MPI programs may require that the programs be run with mpiexec or
    # something similar (this is true for the IBM MPI, for example).
    rm -f f77/init/checksizes.c
    # If it is a VPATH build, f77/init may not be there.
    if test ! -d f77/init ; then
        mkdir f77
        mkdir f77/init
    fi
    cat > f77/init/checksizes.c <<EOF
#include "mpi.h"
#include <stdio.h>
int main( int argc, char **argv )
{
  int fsizeof_aint   = $MPI_SIZEOF_AINT;
  int fsizeof_offset = $MPI_SIZEOF_OFFSET;
  int err = 0, rc = 0;

  MPI_Init( &argc, &argv );
  if (sizeof(MPI_Aint) != fsizeof_aint) {
     printf( "Sizeof MPI_Aint is %d but Fortran thinks it is %d\n",
             (int)sizeof(MPI_Aint), fsizeof_aint );
     err++;
  }
  if (sizeof(MPI_Offset) != fsizeof_offset) {
     printf( "Sizeof MPI_Offset is %d but Fortran thinks it is %d\n",
             (int)sizeof(MPI_Offset), fsizeof_offset );
     err++;
  }
  MPI_Finalize( );
  if (err > 0) rc = 1;
  return rc;
}
EOF

    # Check for name mapping so that we can do the handle conversion tests
    # This test needs both the base c and fortran compilers
    AC_LANG_FORTRAN77
    PAC_PROG_F77_NAME_MANGLE

    # Check whether we need -lU77 to get iargc and getarg, which
    # are used for a few of the tests in spawn (U77 was needed with
    # the native compilers on HPUX.  See the aclocal_f77(old).m4 file,
    # which has a much more complete set of tests.
    #
    # FIXME: if we can't figure out how to get iargc/getarg, then
    # we should really turn off those spawn tests
    # Even better is to limit this to the F200x version, where there is
    # an interface to the command line.
    F77SPAWNARGTEST=""
    AC_MSG_CHECKING([for Fortran libraries needed for getarg])
    AC_LANG_CONFTEST([
        AC_LANG_PROGRAM([],[
            character*64 s
            integer i
            i = iargc()
            call getarg(i,s)
        ])
    ])
    AC_LINK_IFELSE([],[
        pac_cv_getarg_needs_u77=no
    ],[
        pac_cv_getarg_needs_u77=unknown
    ])
    if test "$pac_cv_getarg_needs_u77" != "no" ; then
        # Try again with -lU77
        saveLIBS="$LIBS"
        LIBS="$LIBS -lU77"
        AC_LINK_IFELSE([],[
            pac_cv_getarg_needs_u77=yes
        ],[
            pac_cv_getarg_needs_u77=unavailable
        ])
        LIBS="$saveLIBS"
        if test "$pac_cv_getarg_needs_u77" = "yes" ; then
            F77_GETARG_LIBS=-lU77
        fi
    fi
    rm -f conftest$ac_exeext
    if test -n "$F77_GETARG_LIBS" ; then
        AC_MSG_RESULT($F77_GETARG_LIBS)
    else
        if test "$pac_cv_getarg_needs_u77" = "unavailable" ; then
            AC_MSG_RESULT([getarg and/or iargc are not available.  Some spawn tests will fail to link])
            F77SPAWNARGTEST="#"
        else
            AC_MSG_RESULT([none needed])
        fi
    fi
    AC_SUBST(F77_GETARG_LIBS)
    # FIXME: Currently, we hope that FC accepts the same library
    FC_GETARG_LIBS="$F77_GETARG_LIBS"
    AC_SUBST(FC_GETARG_LIBS)
    # F77SPAWNARGTEST is set to "#" to comment out tests in
    # f77/spawn/testlist.in that require non-standard extensions to
    # access the commandline
    AC_SUBST(F77SPAWNARGTEST)

    # ALLOCMEMF is set in f77/ext/testlist if we can try this
    # Fortran extension
    ALLOCMEMF=""
    PAC_PROG_F77_CRAY_POINTER([
        ALLOCMEMF="allocmemf 1"
        FFLAGS="$FFLAGS $CRAYPTR_FFLAGS"
    ])
    AC_SUBST(ALLOCMEMF)
    # See the f90 block of code for the Fortran 90 version of ALLOCMEMF,
    # i.e. ALLOCMEMFC.

fi
# Set a default for the Fortran 77 version of MPI_Offset.
if test -z "$F77_MPI_OFFSET" ; then
    F77_MPI_OFFSET="integer*8"
    AC_MSG_WARN([Selecting integer*8 as the Fortran type for MPI_Offset])
fi
AC_SUBST(F77_MPI_OFFSET)
# FIXME: Find a way to get the correct value
if test -z "$F77_MPI_ADDRESS" ; then
    F77_MPI_ADDRESS="integer"
    AC_MSG_WARN([Selecting integer as the Fortran type for MPI_Aint])
fi
AC_SUBST(F77_MPI_ADDRESS)

# Running Fortran 90+ compiler tests
AC_PROG_FC
if test "$enable_fc" = yes ; then
    # Work around feature in autoconf that adds -g -O2 to FCFLAGS
    saveFCFLAGS="$FCFLAGS"
    FCFLAGS="$saveFCFLAGS"
    # If there is no working FC, then set enable_fc to no
    if test -z "$FC" ; then
        enable_fc=no
    fi
fi
f90dir="#"
AC_SUBST(f90dir)
# First, see if we have an f90 compiler.  This uses code similar to that
# in the MPICH top-level configure
if test "$enable_fc" = yes -a "$enable_f77" = yes ; then
    PAC_PROG_FC_WORKS
    if test -z "$FC" -o "$pac_cv_prog_fc_works" != yes ; then
        enable_fc=no
    fi
fi

dnl If enable_fc=yes up to this point then enable_f77=yes also
dnl PAC_PROG_FC and PAC_PROG_FC_WORKS return OK
if test "$enable_fc" = yes ; then
    # Make sure that the compilers are compatible.  This
    # will also make sure that the program named in FC is
    # a working Fortran 90 compiler
    PAC_FC_AND_F77_COMPATIBLE(enable_fc=yes,enable_fc=no)
fi

if test "$enable_fc" = yes ; then
    if test "$ac_fc_srcext" != "f90" ; then
        AC_LANG_PUSH([Fortran])
        AC_FC_SRCEXT([f90],[
            FCFLAGS="$FCFLAGS $FCFLAGS_f90"
        ],[
            AC_MSG_WARN([Fortran 90 test being disabled because the $FC compiler does not accept a .f90 extension])
            f90dir=#
            enable_fc=no
        ])
        AC_LANG_POP([Fortran])
    fi

    # The Fortran90 tests rely on free-form input which needs to be tested
    # before any test that may modify FCFLAGS, e.g. the cray-pointer test.
    # The order of the tests is important in compiler like g95.
    # Recent experience showed that the IBM xlf compiler, at least on
    # some systems, requires -qfree=f90 instead of -qfree .  At this
    # writing (11/27/12), this Autoconf macro still uses -qfree and has
    # no mechanism for extension.  This test may fail in that case; if
    # you encounter a problem, document it and submit it to the autoconf
    # bug list, not the MPICH bug list.
    AC_FC_FREEFORM

    # See if the compiler supports the Cray-style pointers
    ALLOCMEMFC=""
    PAC_PROG_FC_CRAY_POINTER([
        ALLOCMEMFC="allocmemf90 1"
        FCFLAGS="$FCFLAGS $CRAYPTR_FCFLAGS"
    ])
    AC_SUBST(ALLOCMEMFC)

    # Check for the new command line routines used in one of the spawn tests
    AC_LANG_PUSH([Fortran])
    F03SPAWNARGTEST=""
    AC_MSG_CHECKING([whether $FC supports the Fortran 2003 routines to access the commandline])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([],[
            character*64 s
            integer i
            i = command_argument_count()
            call get_command_argument(i,s)
        ])],[pac_cv_fc_has_args=yes],[pac_cv_fc_has_args=no])
    AC_MSG_RESULT($pac_cv_fc_has_args)
    if test "$pac_cv_fc_has_args" != "yes" ; then
        F03SPAWNARGTEST="#"
    fi
    # F03SPAWNARGTEST is set to "#" to comment out tests in
    # f90/spawn/testlist.in that require Fortran 2003 features to
    # access the commandline
    AC_SUBST(F03SPAWNARGTEST)
    AC_LANG_POP([Fortran])

    if test -f f77/init/checksizes.c ; then
        # If it is a VPATH build, f90/init may not be there.
        if test ! -d f90/init ; then
            mkdir f90
            mkdir f90/init
        fi
        cp f77/init/checksizes.c f90/init
    fi
    # At least one of the Fortran 90 tests makes use of a module; this
    # allows us to find it to delete the created object file
    PAC_FC_MODULE
fi
#
if test "$enable_fc" = yes ; then
    AC_MSG_CHECKING([that we can build MPI programs with Fortran 90])
    AC_LANG_PUSH([Fortran])
    AC_LINK_IFELSE([
        AC_LANG_SOURCE([
            program main
            use mpi
            integer ierr
            call mpi_init(ierr)
            call mpi_finalize(ierr)
            end
        ])
    ],[
        AC_MSG_RESULT(yes)
        otherlangs="$otherlangs f90"
        f90dir=f90
    ],[
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([Fortran])
fi

# The f77/profile tests will failed to link if we are doing static linking and using
# libmpi+libpmpi. The compiler will complain of multiple definition of the profiled
# profiled routines. It still can be built if we pass the -Wl,--allow-multiple-definition
# flag. For simplicity, we disable the test for this case.
#
f77profile=
AC_SUBST(f77profile)
if test "$f77dir" = "f77" ; then
    AC_MSG_CHECKING([that f77/profile tests can be built])
    AC_LANG_PUSH([Fortran])
    AC_LINK_IFELSE([
        AC_LANG_SOURCE([
            program main
            integer ierr
            integer msg(1)
            call mpi_init(ierr)
            call mpi_send(msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, ierr)
            call mpi_finalize(ierr)
            end

            subroutine mpi_send(msg, count, dtype, dest, tag, comm, ierr)
            include "mpif.h"
            integer count, dtype, dest, tag, comm, ierr
            integer msg(count)
            call pmpi_send(msg, count, dtype, dest, tag, comm, ierr)
            return
            end
        ])
    ],[
        AC_MSG_RESULT(yes)
        f77profile="profile"
    ],[
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([Fortran])
fi

# The f90/profile tests uses the mechanism of e.g. "use mpi, skip_mpi_send => mpi_send".
# If mpi.mod doesn't contain the routine interface, for example, when ignore TKR isn't
# available, it need use the f77 mechanism of direct linking. For simplicity, here we
# disable the profile test if that's the case.
#
f90profile=
AC_SUBST(f90profile)
if test "$f90dir" = "f90" ; then
    AC_MSG_CHECKING([that mpi module contains choice buffer routines])
    AC_LANG_PUSH([Fortran])
    AC_COMPILE_IFELSE([
        AC_LANG_SOURCE([
            program main
            use mpi, skip_mpi_send => mpi_send
            end
        ])
    ],[
        AC_MSG_RESULT(yes)
        f90profile="profile"
    ],[
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([Fortran])
fi

f08dir="#"
AC_SUBST(f08dir)
if test "$enable_f08" = "yes" ; then
    AC_MSG_CHECKING([that we can build MPI programs with use mpi_f08])
    AC_LANG_PUSH([Fortran])
    AC_LINK_IFELSE([
        AC_LANG_SOURCE([
            program main
            use mpi_f08
            integer ierr
            call mpi_init(ierr)
            call mpi_finalize(ierr)
            end
        ])
    ],[
        AC_MSG_RESULT(yes)
        f08dir=f08
    ],[
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([Fortran])
fi

# Running C++ compiler tests
AC_PROG_CXX
if test "$enable_cxx" = yes ; then
    AC_MSG_CHECKING([that we can build MPI programs with CXX binding])
    AC_LANG_PUSH([C++])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"], [
            MPI::Init(NULL, NULL);
            MPI::Finalize();
        ])
    ],[
        AC_MSG_RESULT(yes)
    ],[
        enable_cxx=no
        AC_MSG_RESULT(no)
    ])
    AC_LANG_POP([C++])
fi
# Simple tests for which other languages we can handle
cxxdir="#"
# The C++ interface added support for the Distgraph routines in MPI-2.2,
# but not all MPI implementations support that.  nocxxdistgraph allows
# us to detect that and to skip the test when it is not supported.
nocxxdistgraph="#"
AC_SUBST(cxxdir)
if test "$enable_cxx" = yes ; then
    AC_MSG_CHECKING([that we can build MPI programs with C++])
    AC_LANG_PUSH([C++])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
#include "mpi.h"
        ],[
    MPI::Init();
    MPI::Finalize();
        ])
    ],[
        AC_MSG_RESULT(yes)
        otherlangs="$otherlangs cxx"
        cxxdir=cxx
    ],[
        AC_MSG_RESULT(no)
    ])
    # Check for support of the Distgraphcomm, added in MPI 2.2.
    # Some MPI implementations may support MPI 2.2 or MPI 3.x, but not
    # support the Distgraphcomm C++ interface
    AC_MSG_CHECKING([whether MPI C++ includes Distgraphcomm])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([
#include "mpi.h"
        ],[
    MPI::Distgraphcomm dcomm;
    MPI::Init();
    MPI::Finalize();
        ])
    ],[
        AC_MSG_RESULT(yes)
        nocxxdistgraph=""
    ],[
        AC_MSG_RESULT(no)
    ])

    AC_LANG_POP([C++])
fi
AC_SUBST(nocxxdistgraph)

if test "$enable_cxx" = yes ; then
    AC_CACHE_CHECK([whether <iostream> available],pac_cv_cxx_has_iostream,[
    AC_LANG_PUSH([C++])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include <iostream>],[using namespace std;])
    ],[pac_cv_cxx_has_iostream=yes],[pac_cv_cxx_has_iostream=no])
    ])

    if test "$pac_cv_cxx_has_iostream" = yes ; then
        AC_DEFINE(HAVE_IOSTREAM,1,[Define if iostream is available])
    else
        # Look for iostream.h (in C++ mode, we need the full name)
        AC_CHECK_HEADERS(iostream.h)
        if test "$ac_cv_header_iostream_h" != yes ; then
            AC_MSG_ERROR([C++ compiler $CXX $CXXFLAGS has neither iostream nor iostream.h.])
        fi
        # Warning: the autoconf macros will fall back onto /lib/cpp for
        # C and C++ preprocessing *without* checking that /lib/cpp even
        # exists.
        if test "$CXXCPP" = "/lib/cpp" ; then
            if test ! -x "/lib/cpp" ; then
                AC_MSG_WARN([Warning: Autoconf error, could not find a C++ Preprocessor.  Using false for the preprocessor so that tests will continue.])
                CXXCPP=false
            fi
        fi
    fi

    AX_CXX_NAMESPACE_STD

    if test "$ac_cv_cxx_namespaces" != yes ; then
        AC_MSG_WARN([The compiler $CXX does not support C++ namespaces.  This may cause problems for the tests])
    fi
    AC_LANG_POP([C++])

    dnl for util/mtest_cxx.cxx
    if test "$mpi_io_works" = "yes" ; then
        AC_DEFINE(HAVE_MPI_IO,1,[Define if MPI-IO (really ROMIO) is included])
    fi
fi

AC_LANG_C

LT_INIT()

# IO
iodir="#"
if test "$enable_romio" != no -a "$mpi_io_works" = yes ; then
    iodir=io

    dnl for io/async.c
    AC_CACHE_CHECK([whether MPIO_Request is defined for MPI IO],
    pac_cv_have_mpio_request,[
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([#include "mpi.h"],[MPIO_Request r;])
        ],[pac_cv_have_mpio_request=yes],[pac_cv_have_mpio_request=no])
    ])
    if test "$pac_cv_have_mpio_request" = no ; then
        AC_DEFINE(MPIO_USES_MPI_REQUEST,,[Define if MPI IO uses MPI_Request])
    fi
fi
AC_SUBST(iodir)

impldir=""
#
# Check for implementation to enable implementation-specific options
if test $enable_strictmpi != "yes" ; then
    if test "$MPI_IS_MPICH" = "yes" ; then
        impldir="mpich"
    fi
fi
AC_SUBST(impldir)

pmidir=""
PAC_PUSH_FLAG([LIBS])
LIBS="$LIBS -lpmi"
AC_MSG_CHECKING([that we have pmi library])
AC_LINK_IFELSE([
    AC_LANG_PROGRAM([#include "pmi.h"], [
        int has_parent;
        PMI_Init(&has_parent);
        PMI_Finalize();
    ])
],[
    AC_MSG_RESULT(yes)
    pmidir=pmi
],[
    AC_MSG_RESULT(no)
])
PAC_POP_FLAG([LIBS])
AC_SUBST(pmidir)

#
# MPI_INTEGER16 is mentioned in only one place in MPI 2.1, and some
# implementations may have missed it.  Check to see if it is available in
# C.
AC_CACHE_CHECK([whether MPI_INTEGER16 is available],
pac_cv_have_mpi_integer16,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_Datatype t = MPI_INTEGER16;
        ])
    ],[pac_cv_have_mpi_integer16=yes],[pac_cv_have_mpi_integer16=no])
])
if test "$pac_cv_have_mpi_integer16" = yes ; then
    AC_DEFINE(HAVE_MPI_INTEGER16,1,[Define if MPI_INTEGER16 is available])
fi

# Check if MPI_REAL16 and MPI_COMPLEX32 are defined. Tests handle the case
# where they are defined, but the value is the same as MPI_DATATYPE_NULL.
AC_CACHE_CHECK([whether MPI_REAL16 is available],
pac_cv_have_mpi_real16,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_Datatype t = MPI_REAL16;
        ])
    ],[pac_cv_have_mpi_real16=yes],[pac_cv_have_mpi_real16=no])
])
if test "$pac_cv_have_mpi_real16" = yes ; then
    AC_DEFINE(HAVE_MPI_REAL16,1,[Define if MPI_REAL16 is available])
fi
AC_CACHE_CHECK([whether MPI_COMPLEX32 is available],
pac_cv_have_mpi_complex32,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_Datatype t = MPI_COMPLEX32;
        ])
    ],[pac_cv_have_mpi_complex32=yes],[pac_cv_have_mpi_complex32=no])
])
if test "$pac_cv_have_mpi_complex32" = yes ; then
    AC_DEFINE(HAVE_MPI_COMPLEX32,1,[Define if MPI_COMPLEX32 is available])
fi

# MPI_Aint was intended as an address-sized int.  However, MPI didn't
# specify this - MPI_Aint must be large enough to hold an address-sized
# integer, but it can be larger.  To get clean compilation in some places,
# we need a pointer-sized integer.  The following code looks for one.

# Make sure we are using the local C compiler (if the local
# machine is different that the system that MPICC is for, then
# set the cross-compilation feature)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(short)
POINTERINT=MPI_Aint
AC_MSG_CHECKING([for C integer type of address size])
for type in int long long_long short ; do
    eval len=\$ac_cv_sizeof_$type
    if test "$len" = "$ac_cv_sizeof_void_p" ; then
        POINTERINT=`echo $type | sed -e 's/_/ /'`
        break
    fi
done
AC_MSG_RESULT($POINTERINT)
AC_DEFINE_UNQUOTED(POINTERINT_t,$POINTERINT,[POINTERINT_t is a pointer-sized integer])

# Find perl; used to create some of the tests from template and
# definition files
AC_PATH_PROG(PERL,perl)
AC_SUBST(PERL)
AC_SUBST(otherlangs)
AC_SUBST(threadsdir)
AC_SUBST(MAKE)
if test -z "$MPILIBNAME" ; then MPILIBNAME=mpich ; fi
AC_SUBST(MPILIBNAME)

# TODO: use variables such as has_f77 etc. instead of double use $f77dir etc.
AM_CONDITIONAL([HAS_F77], [test $f77dir = f77])
AM_CONDITIONAL([HAS_F90], [test $f90dir = f90])
AM_CONDITIONAL([HAS_F08], [test $f08dir = f08])
AM_CONDITIONAL([HAS_CXX], [test $cxxdir = cxx])

AC_CONFIG_COMMANDS([chmod],[
    chmod a+x maint/testmerge
    chmod a+x runtests checktests
    chmod a+x manual/manyconnect
    chmod a+x impls/hydra/proc_binding.sh
])
dnl Note that this format for AC_OUTPUT can cause problems for autoconf
dnl run under cygwin
AC_OUTPUT(maint/testmerge \
          checktests \
          Makefile \
          basic/Makefile \
          attr/Makefile \
          attr/testlist \
          util/Makefile \
          coll/Makefile \
          coll/testlist \
          comm/Makefile \
          comm/testlist \
          datatype/Makefile \
          datatype/testlist \
          errhan/Makefile \
          group/Makefile \
          info/Makefile \
          init/Makefile \
          pt2pt/Makefile \
          pt2pt/testlist \
          part/Makefile  \
          mpi_t/Makefile \
          rma/Makefile \
          rma/testlist \
          session/Makefile \
          spawn/Makefile \
          spawn/testlist \
          topo/Makefile \
          io/Makefile \
          io/testlist \
          f77/Makefile \
          f77/attr/Makefile \
          f77/attr/attraints.h \
          f77/pt2pt/attr1aints.h \
          f77/ext/add1size.h \
          f77/datatype/Makefile \
          f77/datatype/typeaints.h \
          f77/coll/Makefile \
          f77/comm/Makefile \
          f77/topo/Makefile \
          f77/init/Makefile \
          f77/rma/addsize.h \
          f77/pt2pt/Makefile \
          f77/info/Makefile \
          f77/spawn/Makefile \
          f77/spawn/testlist \
          f77/spawn/type1aint.h \
          f77/rma/Makefile \
          f77/ext/Makefile \
          f77/ext/testlist \
          f77/io/Makefile \
          f77/io/iooffset.h \
          f77/io/iodisp.h \
          f77/io/ioaint.h \
          f77/io/testlist \
          f77/profile/Makefile \
          f90/Makefile \
          f90/attr/Makefile \
          f90/datatype/Makefile \
          f90/f90types/Makefile \
          f90/coll/Makefile \
          f90/comm/Makefile \
          f90/topo/Makefile \
          f90/init/Makefile \
          f90/pt2pt/Makefile \
          f90/rma/Makefile \
          f90/info/Makefile \
          f90/spawn/Makefile \
          f90/spawn/testlist \
          f90/timer/Makefile \
          f90/ext/Makefile \
          f90/ext/testlist \
          f90/io/Makefile \
          f90/io/testlist \
          f90/misc/Makefile \
              f90/profile/Makefile \
          f08/Makefile \
              f08/attr/Makefile \
              f08/datatype/Makefile \
              f08/coll/Makefile \
              f08/comm/Makefile \
              f08/pt2pt/Makefile \
              f08/rma/Makefile \
              f08/subarray/Makefile \
              f08/topo/Makefile \
              f08/io/Makefile \
              f08/io/testlist \
          f08/init/Makefile \
          f08/info/Makefile \
          f08/spawn/Makefile \
          f08/spawn/testlist \
          f08/timer/Makefile \
          f08/ext/Makefile \
          f08/misc/Makefile \
          f08/profile/Makefile \
     cxx/Makefile \
          cxx/attr/Makefile \
          cxx/attr/testlist \
          cxx/pt2pt/Makefile \
          cxx/comm/Makefile \
          cxx/coll/Makefile \
          cxx/errhan/Makefile \
          cxx/info/Makefile \
          cxx/datatype/Makefile \
          cxx/datatype/testlist \
          cxx/io/Makefile \
          cxx/init/Makefile \
          cxx/rma/Makefile \
          cxx/spawn/Makefile \
          cxx/spawn/testlist \
          cxx/topo/Makefile \
          threads/Makefile \
          threads/pt2pt/Makefile \
          threads/comm/Makefile \
          threads/comm/testlist \
          threads/init/Makefile \
          threads/mpi_t/Makefile \
          threads/spawn/Makefile \
          threads/rma/Makefile \
          threads/coll/Makefile \
          threads/coll/testlist \
          threads/perf/Makefile \
          threads/part/Makefile  \
          errors/Makefile \
          errors/attr/Makefile \
          errors/basic/Makefile \
          errors/coll/Makefile \
          errors/comm/Makefile \
          errors/datatype/Makefile \
          errors/faults/Makefile \
          errors/group/Makefile \
          errors/pt2pt/Makefile \
          errors/rma/Makefile \
          errors/session/Makefile \
          errors/spawn/Makefile \
          errors/spawn/testlist \
          errors/topo/Makefile \
          errors/io/Makefile \
          errors/cxx/Makefile \
          errors/cxx/errhan/Makefile \
          errors/cxx/io/Makefile \
          errors/f77/Makefile \
          errors/f77/io/Makefile \
          errors/f77/io/addsize.h \
          errors/f77/io/iooffset.h \
          errors/f90/Makefile \
          errors/f90/io/Makefile \
          errors/f08/Makefile \
          errors/f08/io/Makefile \
          ckpoint/Makefile \
          ft/Makefile \
          manual/Makefile \
          manual/manyconnect \
          manual/mpi_t/Makefile \
          perf/Makefile \
          testlist \
          cxx/testlist \
          cxx/topo/testlist \
          f77/testlist \
          f90/testlist \
          f08/testlist \
          threads/testlist \
          errors/testlist \
          errors/cxx/testlist \
          errors/f77/testlist \
          errors/f90/testlist \
          impls/testlist \
          f77/rma/testlist \
          f90/rma/testlist \
          f08/rma/testlist \
          impls/Makefile \
          impls/hydra/Makefile \
          impls/hydra/proc_binding.sh \
          impls/pmi/Makefile \
          impls/mpich/Makefile \
          impls/mpich/testlist \
          impls/mpich/mpi_t/Makefile \
          impls/mpich/comm/Makefile \
          impls/mpich/comm/testlist \
          impls/mpich/threads/Makefile \
          impls/mpich/threads/testlist \
          impls/mpich/threads/pt2pt/Makefile \
          impls/mpich/threads/threadcomm/Makefile \
          impls/mpich/cuda/Makefile \
          impls/mpich/hip/Makefile \
          impls/mpich/misc/Makefile \
          impls/mpich/ulfm/Makefile \
          )

