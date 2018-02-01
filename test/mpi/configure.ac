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

AC_CONFIG_HEADER(include/mpitestconf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
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
echo "RUNNING CONFIGURE FOR MPI TESTS"

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
    # Now, see if we can find the f77tof90 routine
    if test -x $use_top_srcdir/../../maint/f77tof90 ; then 
        master_top_srcdir=`cd $use_top_srcdir && cd ../.. && pwd`
    else 
        AC_MSG_ERROR([Unable to find master source file - reconfigure using --disable-maintainer_mode])
    fi
fi

AC_ARG_ENABLE(echo,
	[AC_HELP_STRING([--enable-echo],[Turn on strong echoing. The default is enable=no.])],
	[set -x])

AC_ARG_ENABLE(fortran,
[  --enable-fortran=option - Control the level of Fortran support in the MPICH implementation.
        yes|all   - Enable all available Fortran implementations (F77, F90, F08)
        f77       - Enable Fortran 77 support
        f90       - Enable Fortran 90 support
        f08       - Enable Fortran 2008 support
        no|none   - No Fortran support
],,[enable_fortran=f77,f90])

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
        f77)
                enable_f77=yes
                ;;
        f90)
                enable_fc=yes
                ;;
        f08)
                enable_f08=yes
                ;;
        *)
                IFS="$save_IFS"
                AC_MSG_WARN([Unknown value $option for --enable-fortran])
                IFS=","
                ;;
    esac
done
IFS="$save_IFS"

AC_ARG_ENABLE(cxx,
	[AC_HELP_STRING([--enable-cxx],[Turn on C++ tests (default)])],,[enable_cxx=yes])

AC_ARG_ENABLE(romio,
	[AC_HELP_STRING([--enable-romio],[Enable ROMIO MPI I/O implementation])],,
	[enable_romio=yes])

AC_ARG_ENABLE(spawn,
	[AC_HELP_STRING([--enable-spawn],
		[Enable tests of the dynamic process parts of MPI-2 (default)])],,
	[enable_spawn=yes])

AC_ARG_ENABLE(rma,
	[AC_HELP_STRING([--enable-rma],[Enable tests of the one sided parts of MPI-2 (default)])],,
	[enable_rma=yes])

AC_ARG_ENABLE(long-double-complex,
	[AC_HELP_STRING([--enable-long-double-complex],
		[Enable tests involving MPI_LONG_DOUBLE_COMPLEX (default)])],,
	[enable_long_double_complex=yes])
AC_ARG_ENABLE(long-double,
	[AC_HELP_STRING([--enable-long-double-complex],
		[Enable tests involving MPI_LONG_DOUBLE and related types (default)])],,
	[enable_long_double=yes])

AC_ARG_ENABLE(checkerrors,
	[AC_HELP_STRING([--enable-checkerrors],
		[Add some tests for checking for errors in user programs])],,
	[enable_checkerrors=yes])

AC_ARG_ENABLE(perftest,
	[AC_HELP_STRING([--enable-perftest],
		[Include tests for basic performance consistency (default)])],,
	[enable_perftest=yes])

AC_ARG_ENABLE(large-tests,
	[AC_HELP_STRING([--enable-large-tests],
		[Enable tests which require large (>2GB per proc) amount of memory])],,
	[enable_large_tests=no])

AC_ARG_ENABLE(ft-tests,
	[AC_HELP_STRING([--enable-ft-tests],
		[Include tests for fault tolerance (default)])],,
	[enable_ft_tests=yes])

AC_ARG_ENABLE(comm-overlap-tests,
	[AC_HELP_STRING([--enable-comm-overlap-tests],
		[Include tests for communicator overlap (default)])],,
	[enable_comm_overlap_tests=yes])

AC_ARG_ENABLE(checkfaults,
	[AC_HELP_STRING([--enable-checkfaults],
		[Add some tests for checking on handling of faults in user programs])],,
	[enable_checkfaults=no])

AC_ARG_ENABLE(checkpointing,
	[AC_HELP_STRING([--enable-checkpointing],
		[Add some tests for checkpointing])],,
	[enable_checkpointing=no])

AC_ARG_ENABLE(fast,
	[AC_HELP_STRING([--enable-fast],
		[Indicates that the MPI implementation may have been
		built for fastest operation, such as building without
		error checking.  Has the effect of
		--enable-checkerrors=no])],,)

AC_ARG_ENABLE(strictmpi,
	[AC_HELP_STRING([--enable-strictmpi],
		[Only test for operations specifically defined by the
		MPI standard.  This turns off tests for some common
		extensions, including for combinations of predefined
		datatypes and predefined MPI_Op s.])],,
	[enable_strictmpi=no])

AC_ARG_ENABLE(threads,
	[--enable-threads=level - Specify the level of thread support expected from the
				MPI implementation.  The following levels are supported.

	single - No threads (MPI_THREAD_SINGLE)
	funneled - Only the main thread calls MPI (MPI_THREAD_FUNNELED)
	serialized - User serializes calls to MPI (MPI_THREAD_SERIALIZED)
	multiple - Fully multi-threaded (MPI_THREAD_MULTIPLE)

	The default is funneled.  If enabled and no level is
	specified, the level is set to multiple.  If disabled, the
	level is set to single.  If the environment variable
	MPICH_THREAD_LEVEL is set, that thread level is used (this is
	to let MPICH build for the correct thread support without
	requiring a specific --enable-threads argument.],,
	[enable_threads=default])

AC_ARG_ENABLE(xfail,
	[AC_HELP_STRING([--enable-xfail],
		[Run tests marked for expected failure])],,
	[enable_xfail=no])

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

# First, determine whether we are/can support the language bindings
#
# Since F90/F90FLAGS are replaced by FC/FCFLAGS, rather than silently
# substituting them, i.e. FC=$F90 and FCFLAGS=$F90FLAGS, we choose to emit
# an error message and abort to avoid any ambiguous/hidden bug in choosing
# Fortran90 compilers.
if test -n "$F90" -o -n "$F90FLAGS" ; then
    AC_MSG_ERROR([F90 and F90FLAGS are replaced by FC and FCFLAGS respectively in this configure, please unset F90/F90FLAGS and set FC/FCFLAGS instead and rerun configure again.])
fi

# ------------------------------------------------------------------------
dnl use AC_ARG_VAR to mark FROM_MPICH as "precious" to autoconf so that
dnl automatic re-runs of config.status preserve its value correctly
AC_ARG_VAR([FROM_MPICH],[should be set to "yes" if this configure script is being invoked by the main MPICH configure])
AC_ARG_VAR([MPICH_THREAD_LEVEL],
           [the MPI thread level supported by the enclosing MPICH build (when built within MPICH)])
# ------------------------------------------------------------------------

if test "$enable_threads" = "yes" ; then 
    enable_threads=multiple
elif test "$enable_threads" = "no" ; then
    enable_threads=single
elif test "$enable_threads" = default ; then
    if test -n "$MPICH_THREAD_LEVEL" ; then
        case $MPICH_THREAD_LEVEL in 
            MPI_THREAD_MULTIPLE)
            enable_threads=multiple
            ;;
            MPI_THREAD_SERIALIZED)
            enable_threads=serialized
            ;;
            MPI_THREAD_FUNNELED)
            enable_threads=funneled
            ;;
            MPI_THREAD_SINGLE)
            enable_threads=single
            ;;
        esac
    else
        enable_threads=funneled
    fi
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

# 
# Only run the threads tests if multiple is specified
if test "$enable_threads" = "multiple" -o "$enable_threads" = "runtime" ; then
    threadsdir="threads"
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

# more variables that must be marked precious for proper re-configure operation
AC_ARG_VAR([MPICH_ENABLE_F77],["yes" if the enclosing MPICH build supports Fortran 77])
AC_ARG_VAR([MPICH_ENABLE_FC],["yes" if the enclosing MPICH build supports modern Fortran])
AC_ARG_VAR([MPICH_ENABLE_CXX],["yes" if the enclosing MPICH build supports C++])

# If we're building from MPICH, check the MPICH_ENABLE_xxx environment
# variables for enable defaults
if test "$FROM_MPICH" = yes ; then
    if test -n "$MPICH_ENABLE_F77" ; then
        enable_f77=$MPICH_ENABLE_F77
    fi
    if test -n "$MPICH_ENABLE_FC" ; then
        enable_fc=$MPICH_ENABLE_FC
    fi
    if test -n "$MPICH_ENABLE_CXX" ; then
        enable_cxx=$MPICH_ENABLE_CXX
    fi
    namepub_tests="#"
    if test -n "$nameserv_name" ; then
        namepub_tests=""
    fi
    AC_SUBST(namepub_tests)
fi

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

faultsdir=#
if test "$enable_checkfaults" = "yes" ; then
    faultsdir=faults
fi
AC_SUBST(faultsdir)
#
MPI_IS_STRICT=false
AC_SUBST(MPI_IS_STRICT)
if test "$enable_strictmpi" = "yes" ; then
    MPI_IS_STRICT=true
    AC_DEFINE(USE_STRICT_MPI,1,[Define if only operations defined in MPI should be tested])
fi
#
# At this writing, MPICH has many MPIX routines, and the test suite includes
# them.  As these are not MPI routines (yet), they are invalid and incorrect
# when this test suite is used for other MPI implementations, including those
# based on earlier versions of MPICH.
MPI_HAS_MPIX=no
#
# Hack to detect build from within MPICH.  Ensure strictmpi not selected.
if test "$FROM_MPICH" = "yes" -a "$enable_strictmpi" = "no" ; then
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

# preserve these values across a reconfigure
AC_ARG_VAR([WRAPPER_CFLAGS],[])
AC_ARG_VAR([WRAPPER_CPPFLAGS],[])
AC_ARG_VAR([WRAPPER_LDFLAGS],[])
AC_ARG_VAR([WRAPPER_LIBS],[])
AC_ARG_VAR([WRAPPER_FFLAGS],[])
AC_ARG_VAR([WRAPPER_FCFLAGS],[])
AC_ARG_VAR([WRAPPER_CXXFLAGS],[])

# Attach program prefix and suffix to executable names
PAC_GET_EXENAME("mpicc", MPICC_NAME)
PAC_GET_EXENAME("mpif77", MPIF77_NAME)
PAC_GET_EXENAME("mpifort", MPIFORT_NAME)
PAC_GET_EXENAME("mpicxx", MPICXX_NAME)
PAC_GET_EXENAME("mpiexec", MPIEXEC_NAME)

if test "$FROM_MPICH" = "yes" ; then
    # perform configure tests with the normal compilers ($CC/$F77/etc), but use
    # the WRAPPER_xFLAGS computed by MPICH as our flags instead.  Then at the
    # end of configure we will empty out these flags and set our compilers to
    # the installed compiler wrappers
    CFLAGS="$WRAPPER_CFLAGS"
    CPPFLAGS="$WRAPPER_CPPFLAGS"
    LDFLAGS="$WRAPPER_LDFLAGS"
    FFLAGS="$WRAPPER_FFLAGS"
    FCFLAGS="$WRAPPER_FCFLAGS"
    CXXFLAGS="$WRAPPER_CXXFLAGS"

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
        AC_PATH_PROG(MPIEXEC,$MPIEXEC_NAME)
    fi
fi

# Running C compiler tests
PAC_PROG_CC
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

# not using libtool for the test suite, so no LT_INIT.  Instead, test here
# for Library programs
AC_PROG_RANLIB
AM_PROG_AR

# Enable STRICT CFLAGS
AC_ARG_ENABLE(stricttest,
AC_HELP_STRING([--enable-stricttest], [Turn on strict GCC compilation]))
dnl Initialize enable_strict_done so PAC_CC_STRICT won't exit right away
dnl When it is configuring from within MPICH, because enable_strict_done=yes
dnl is set in the environment by MPICH.
enable_strict_done=no
PAC_CC_STRICT($enable_stricttest)
# -Wfloat-equal isn't meaningful in testsuite,
# remove it if it is in strict flags.
pac_cc_strict_flags="`echo $pac_cc_strict_flags | sed -e 's|-Wfloat-equal||g'`"
CFLAGS="$CFLAGS $pac_cc_strict_flags"

# General headers
AC_HEADER_STDC
dnl AC_CHECK_HEADERS(stdarg.h unistd.h string.h stdlib.h memory.h stdint.h)
dnl unistd.h string.h stdlib.h memory.h stdint.h are checked by AC_PROG_CC.
AC_CHECK_HEADERS(stdarg.h sys/time.h sys/resource.h)

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
# for tests that require large mem
largetest="#"
if test "$enable_large_tests" = "yes" -a $ac_cv_sizeof_void_p -ge 8; then
    largetest=""
fi
AC_SUBST(largetest)


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

threadlib=""
# Headers needed for threads tests
if test "$threadsdir" = "threads" ; then
    # Check for needed threads headers and needed and optional routines
    AC_CHECK_HEADERS(pthread.h)

    # we shouldn't actually need to do this, but we are dodging the chicken/egg
    # problem b/c we don't have a working mpicc yet and we temporarily discarded
    # the MPICH-set LIBS (probably the wrong thing to do)
    AC_SEARCH_LIBS([pthread_create],[pthread],[threadlib="-lpthread"])
    AC_SEARCH_LIBS([pthread_barrier_init],[pthread])

    AC_CHECK_FUNCS(pthread_create pthread_yield pthread_barrier_init pthread_barrier_wait)
    if test "$FROM_MPICH" != yes ; then
        AC_CHECK_FUNCS(MPI_Init_thread)
    fi    
fi
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
if test "$FROM_MPICH" != "yes" ; then
    AC_CACHE_CHECK([that MPI program can be compiled],pac_cv_mpi_compile_ok,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[MPI_Init(0,0);MPI_Finalize();])],pac_cv_mpi_compile_ok=yes,pac_cv_mpi_compile_ok=no)])
    if test "$pac_cv_mpi_compile_ok" != yes ; then
        AC_MSG_ERROR([Unable to compile an MPI program containing mpi.h!])
    fi
    AC_CACHE_CHECK([for major version of MPI],pac_cv_mpi_major_version,[
for pac_cv_mpi_major_version in 3 2 1 unknown ; do
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
else
    # We are within the MPICH build.  Extract the MPI versions from 
    # mpi.h.in	
    if test ! -f $mpich_top_srcdir/src/include/mpi.h.in ; then
        AC_MSG_ERROR([Could not find the required mpi.h.in file!])
    fi
    MPI_VERSION=`grep MPI_VERSION $mpich_top_srcdir/src/include/mpi.h.in | \
    		      sed -e 's/#define *MPI_VERSION *\([0-4]\).*/\1/g'`
    MPI_SUBVERSION=`grep MPI_SUBVERSION $mpich_top_srcdir/src/include/mpi.h.in | \
    		      sed -e 's/#define *MPI_SUBVERSION *\([0-9]\).*/\1/g'`
fi

AC_SUBST(MPI_VERSION)
AC_SUBST(MPI_SUBVERSION)

# Running Fortran 77 compiler tests
PAC_PROG_F77
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
if test "$FROM_MPICH" = yes ; then
    if test "$enable_f77" = yes ; then
       # Assume success
        otherlangs="$otherlangs f77"
        f77dir=f77
        buildingF77=yes
    fi
elif test "$enable_f77" = yes ; then
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
        # We have to try and get the size of offset anyway, since 
        # it is not the same as void * (it depends on the available
        # file systems).  Since we want to avoid using the MPI linker,
        # we could do the following:
        #    Use the mpi compiler to compile the file, using the mpi 
        #    header but no MPI calls
        #    Use the regular C linker to link the program
        # However, we do this only if the environment variable BASECC
        # has been provided.  Else we can try the following:
        # use
        #    sed -n 's/typedef \(.*\) MPI_Offset *;/\1/p' mpi.h
        # to extract the type corresponding to MPI_Offset and then
        # just use that.
dnl         AC_CACHE_CHECK([the sizeof MPI_Offset],ac_cv_sizeof_MPI_Offset,[
dnl              ac_cv_sizeof_MPI_Offset=unknown
dnl              rm -f conftest*
dnl              cat >>conftest.c <<EOF
dnl #include "mpi.h"
dnl #include <stdio.h>
dnl int main( int argc, char **argv )
dnl {
dnl     MPI_Offset a;
dnl     FILE *f = fopen("conftestval", "w" );
dnl     if (! f) exit(1);
dnl     fprintf( f, "%ld\n", (long)sizeof(MPI_Offset) );
dnl     fclose(f);
dnl     return 0;
dnl }
dnl EOF
dnl             # FIXME.  Check for BASECC
dnl             # Note: This assumes that CC has been set to the C compiler for
dnl             # MPI Programs, and that either any necessary include paths are 
dnl             # already set or they are in CPPFLAGS (prefered) or CFLAGS.
dnl             if AC_TRY_EVAL(ac_link) && test -s conftest$ac_exeext ; then
dnl                 if ./conftest$ac_exeext ; then
dnl                     #success
dnl                     ac_cv_sizeof_MPI_Offset=`cat conftestval`
dnl                 else
dnl                     # failure
dnl                     AC_MSG_WARN([Unable to run the program to determine the size of MPI_Offset])
dnl                     echo "configure: failed program was:" >&AC_FD_CC
dnl                     cat conftest.c >&AC_FD_CC
dnl                 fi
dnl             else
dnl                 # failure
dnl                 AC_MSG_WARN([Unable to build the program to determine the size of MPI_Offset])
dnl                 echo "configure: failed program was:" >&AC_FD_CC
dnl                 cat conftest.c >&AC_FD_CC
dnl             fi
dnl             rm -f conftest*
dnl         ])
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

    # Check that the Fortran compiler will allow us to pass arguments
    # of different types (e.g., for MPI_Send)
    # Any strict Fortran compiler will require that the arguments be 
    # the same type - currently, the NAG Fortran compiler (nagfor) is known
    # to enforce this.
    # We could set the FFLAGS/FCFLAGS values with the option that disables
    # this check (if we found one), but because that may affect other tests,
    # instead we tell the user and exit.
    PAC_PROG_F77_MISMATCHED_ARGS(addarg,yes)
    if test "X$addarg" != "X" ; then
        # We could add the names of all of the MPI routines that 
        # accept different types.  Instead, we fail cleanly.  
        # Some Fortran compilers allow you to turn off checking for 
        # mismatched arguments for *all* routines.  Adding an argument
	# that turns off checking for *everything* is not something that 
	# configure should do - if the user wants this, they can follow
	# the instructions in the following error message.
	AC_MSG_ERROR([The Fortran compiler $F77 does not accept programs that call the same routine with arguments of different types without the option $addarg.  Rerun configure with FFLAGS=$addarg])
    fi

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
PAC_PROG_FC
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
    # Only check if we're *not* building within MPICH 
    # (MPICH will have made the test)
    # FIXME: Do we want to check only simple routine names 
    # (those without an underscore?)
    if test "$FROM_MPICH" != yes ; then
        PAC_FC_AND_F77_COMPATIBLE(enable_fc=yes,enable_fc=no)
    fi
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
if test "$FROM_MPICH" = yes ; then
    if test "$enable_fc" = yes ; then
        otherlangs="$otherlangs f90"
        f90dir=f90
    fi
elif test "$enable_fc" = yes ; then
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

if test "$enable_f08" = "yes" ; then
    PAC_FC_2008_SUPPORT([enable_f08=yes],[enable_f08=no])
fi

f08dir="#"
AC_SUBST(f08dir)
# FIXME if $FROM_MPICH is no, we should test build an MPI F08 program
if test "$enable_f08" = "yes" -a "$FROM_MPICH" = "yes" ; then
   f08dir=f08
fi

# Running C++ compiler tests
PAC_PROG_CXX
if test "$enable_cxx" = yes ; then
    if test -z "$CXX" ; then
        enable_cxx=no
    fi
fi
# Simple tests for which other languages we can handle
cxxdir="#"
# The C++ interface added support for the Distgraph routines in MPI-2.2,
# but not all MPI implementations support that.  nocxxdistgraph allows
# us to detect that and to skip the test when it is not supported.
nocxxdistgraph="#"
AC_SUBST(cxxdir)
if test "$FROM_MPICH" = yes ; then
    if test "$enable_cxx" = yes ; then
        otherlangs="$otherlangs cxx"
        cxxdir=cxx
        # MPICH ABI removed support for MPI::Distgraphcomm, so
	# nocxxdistgraph is left as #, which comments out the test
	# in cxx/topol/testlist.in
    fi
elif test "$enable_cxx" = yes ; then
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
fi

AC_LANG_C
# IO
iodir="#"
if test "$enable_romio" != no ; then
    iodir=io
    AC_DEFINE(HAVE_MPI_IO,1,[Define if MPI-IO (really ROMIO) is included])
    if test "$FROM_MPICH" = yes ; then
        # MPICH no longer uses and MPIO_Request
        pac_cv_have_mpio_request=no
    else 
        AC_CACHE_CHECK([whether MPIO_Request is defined for MPI IO],
        pac_cv_have_mpio_request,[
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([#include "mpi.h"],[MPIO_Request r;])
            ],[pac_cv_have_mpio_request=yes],[pac_cv_have_mpio_request=no])
        ])
    fi
    if test "$pac_cv_have_mpio_request" = no ; then
        AC_DEFINE(MPIO_USES_MPI_REQUEST,,[Define if MPI IO uses MPI_Request])
    fi
fi
AC_SUBST(iodir)

impldir="#"
#
# Check for implementation to enable implementation-specific options
if test $enable_strictmpi != "yes" ; then
   # Is this MPICH?
   if test "$FROM_MPICH" = yes ; then
      impldir="mpich"
   else
       AC_CACHE_CHECK([Is the MPI derived from MPICH],
       pac_cv_ismpich,[
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include "mpi.h"],[return 1 + MPICH;]),
       pac_cv_ismpich=yes,pac_cv_ismpich=no])])
       if test "$pac_cv_ismpich" = "yes" ; then
           impldir="mpich"
       fi
   fi
fi
AC_SUBST(impldir)
#
# MPI_INTEGER16 is mentioned in only one place in MPI 2.1, and some
# implementations may have missed it.  Check to see if it is available in 
# C.
if test "$FROM_MPICH" = yes ; then
   # MPICH correctly includes this type.
   pac_cv_have_mpi_integer16=yes
else
    AC_CACHE_CHECK([whether MPI_INTEGER16 is available],
    pac_cv_have_mpi_integer16,[
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([#include "mpi.h"],[
                MPI_Datatype t = MPI_INTEGER16;
            ])
        ],[pac_cv_have_mpi_integer16=yes],[pac_cv_have_mpi_integer16=no])
    ])
fi
if test "$pac_cv_have_mpi_integer16" = yes ; then 
    AC_DEFINE(HAVE_MPI_INTEGER16,1,[Define if MPI_INTEGER16 is available])
fi
AC_CACHE_CHECK([whether MPI_2DOUBLE_COMPLEX is available],
pac_cv_have_mpi_2double_complex,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_Datatype t = MPI_2DOUBLE_COMPLEX;
        ])
    ],[
        pac_cv_have_mpi_2double_complex=yes
    ],[
        pac_cv_have_mpi_2double_complex=no
    ])
])
if test "$pac_cv_have_mpi_2double_complex" = yes ; then 
    AC_DEFINE(HAVE_MPI_2DOUBLE_COMPLEX,1,[Define if MPI_2DOUBLE_COMPLEX is available])
fi
# 2COMPLEX was in MPI 1.0 and removed after that.  This allows us to 
# test 2COMPLEX if it is present
AC_CACHE_CHECK([whether MPI_2COMPLEX is available],
pac_cv_have_mpi_2complex,[
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([#include "mpi.h"],[
            MPI_Datatype t = MPI_2COMPLEX;
        ])
    ],[
        pac_cv_have_mpi_2complex=yes
    ],[
        pac_cv_have_mpi_2complex=no
    ])
])
if test "$pac_cv_have_mpi_2complex" = yes ; then 
    AC_DEFINE(HAVE_MPI_2COMPLEX,1,[Define if MPI_2COMPLEX is available])
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
    F77=$mpibindir/$MPIF77_NAME
    FC=$mpibindir/$MPIFORT_NAME
    CXX=$mpibindir/$MPICXX_NAME
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
    FFLAGS="$CRAYPTR_FFLAGS"
    FCFLAGS="$CRAYPTR_FCFLAGS"
    CXXFLAGS=""
else
    # We need either mpiexec or mpirun.  If we don't find them,
    # the user will need to determine how to run a program
    AC_PATH_PROG(MPIEXEC,$MPIEXEC_NAME)
fi

AC_OUTPUT_COMMANDS([chmod a+x maint/testmerge])
AC_OUTPUT_COMMANDS([chmod a+x runtests checktests])
AC_OUTPUT_COMMANDS([chmod a+x manual/manyconnect])
AC_OUTPUT_COMMANDS([chmod a+x impls/hydra/proc_binding.sh])
dnl Note that this format for AC_OUTPUT can cause problems for autoconf
dnl run under cygwin
AC_OUTPUT(maint/testmerge \
          runtests \
          checktests \
          Makefile \
          basic/Makefile \
          attr/Makefile \
          util/Makefile \
          coll/Makefile \
	  coll/testlist \
          comm/Makefile \
          datatype/Makefile \
          datatype/testlist \
          errhan/Makefile \
          group/Makefile \
          info/Makefile \
          init/Makefile \
          pt2pt/Makefile \
          mpi_t/Makefile \
          rma/Makefile \
          rma/testlist \
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
          f77/util/Makefile \
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
          f90/util/Makefile \
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
	      f08/util/Makefile \
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
          cxx/util/Makefile \
          cxx/attr/Makefile \
          cxx/pt2pt/Makefile \
          cxx/comm/Makefile \
          cxx/coll/Makefile \
	  cxx/errhan/Makefile \
          cxx/info/Makefile \
          cxx/datatype/Makefile \
          cxx/io/Makefile \
          cxx/init/Makefile \
          cxx/rma/Makefile \
          cxx/spawn/Makefile \
          cxx/spawn/testlist \
	  cxx/topo/Makefile \
          threads/Makefile \
          threads/pt2pt/Makefile \
          threads/util/Makefile \
          threads/comm/Makefile \
          threads/comm/testlist \
          threads/init/Makefile \
          threads/mpi_t/Makefile \
          threads/spawn/Makefile \
          threads/rma/Makefile \
          threads/coll/Makefile \
          threads/coll/testlist \
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
          impls/mpich/Makefile \
          impls/mpich/testlist \
          impls/mpich/mpi_t/Makefile \
          )

