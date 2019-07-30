AC_PREREQ(2.63)
#
# (C) 2006 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
dnl Process this file with autoconf to produce a configure script.
dnl
dnl aclocal_cache.m4, included by sowing/confdb/aclocal.m4, fixes
dnl bugs in autoconf caching.
dnl
dnl This is a large configure script and it is important to keep it
dnl clearly organized.  In addition, this script must coordinate with
dnl the other modules that can be used to construct MPICH, such as
dnl the communication device and the process manager.  Each of these
dnl may have special features or limitations that other modules or
dnl this configure may need to take into account.  To handle this, there
dnl are xx major steps in this configure script:
dnl
dnl 1. Identify major modules and source any prerequisite scripts
dnl 2. Determine compiler characteristics
dnl 3. Setup and configure the other modules
dnl 4. Determine MPI features and characteristics (such as datatype values)
dnl
dnl Each of these is described in more detail below.
dnl
dnl 1. Identify the modules (most are specified by
dnl --with-<modulename>=instance,
dnl for example, --with-pm=hydra or --with-device=ch3:nemesis).
dnl For each module, source the file mpichprereq if present (in the
dnl module's top-level directory).  This
dnl must be a bourne (sh) shell script; it can access any of the variables
dnl in the configure script.  In addition, there are a few variables that
dnl are defined and provided to allow the modules to communicate their
dnl needs or limitations to the other modules.  These are:
dnl    MPID_MAX_THREAD_LEVEL - thread level supported by device.
dnl                            if unset, is MPI_THREAD_FUNNELED
dnl    MPID_PM_NAMESERVER    - if set, provides the name of the nameserver
dnl                            that the process manager supports.
dnl                            This name server will be used if the
dnl                            default name server is selected.
dnl    MPID_NO_PM            - If yes, the device does not require any
dnl                            PM implementation.
dnl    MPID_MAX_PROCESSOR_NAME - The maximum number of character in a processor
dnl                            name.  If not set, 128 will be used.
dnl    MPID_MAX_ERROR_STRING - The maximum number of character in an error
dnl                            string.  If not set, 1024 will be used.
dnl    PM_REQUIRES_PMI       - if set, provides the name of the PMI
dnl                            interface implementation.  If not set,
dnl                            the "simple" PMI implementation is used.
dnl                            A process manager that needs a particular
dnl                            process manager should check that this is
dnl                            not set to an incompatible value.
dnl    MPID_NO_SPAWN         - if yes, the device does not support the
dnl                            dynamic process routines (spawn, connect
dnl                            attach, join, plus port and publish
dnl                            routines).  The major effect of this
dnl                            is to let the test codes know that
dnl                            spawn is not implemented.
dnl    MPID_NO_RMA           - if yes, the device does not support the
dnl                            MPI RMA routines (MPI_Win_create and
dnl                            MPI_Put etc.).  The major effect of this
dnl                            is to let the test codes know that
dnl                            RMA is not implemented.
dnl
dnl Note that the meanings of these variables are defined so that an
dnl undefined value gives the default.  This makes it easy to expand
dnl the set of such variables, since only modules that need the new
dnl variable will need to be changed.
dnl
dnl 2. Determine compiler characteristics
dnl Here is where features of the compilers are determined, including
dnl support for shared libraries and sizes of the basic datatype types.
dnl
dnl 3. Setup and configure the other modules
dnl Before each module configure is executed, the script setup_<module>
dnl is run if present.  This is a bourne (sh) shell script and may
dnl access configure variables.  It should not make any changes to the
dnl compiler name or flags (e.g., do not add -D_XOPEN_SOURCE to CFLAGS here,
dnl because that may invalidate the determination of the compiler
dnl characteristics in the prior step).
dnl
dnl 4. Determine MPI features
dnl
dnl
dnl Special environment variables
dnl To let other scripts and in particular the configure in test/mpi
dnl know that they are being invoked from within the MPICH configure,
dnl the following environment variables are set and exported:
dnl    FROM_MPICH
dnl    MPICH_ENABLE_F77
dnl    MPICH_ENABLE_FC
dnl    MPICH_ENABLE_CXX
dnl
dnl Note that no executable statements are allowed (and any are silently
dnl dropped) before AC_INIT.

m4_include([maint/version.m4])
dnl 2nd arg is intentionally underquoted
AC_INIT([MPICH],
        MPICH_VERSION_m4,
        [discuss@mpich.org],
        [mpich],
        [http://www.mpich.org/])

if test "x$prefix" != "xNONE" && test -d "$prefix"; then
    if test "x`(cd \"$prefix\"; echo \"$PWD\")`" = "x`(cd \"$srcdir\"; echo \"$PWD\")`" ||\
       test "x`(cd \"$prefix\"; echo \"$PWD\")`" = "x$PWD"  ; then
        AC_MSG_ERROR([The install directory (--prefix=) cannot be the same as the build or src directory.])
    fi
fi

CONFIGURE_ARGS_CLEAN=`echo $* | tr '"' ' '`
AC_SUBST(CONFIGURE_ARGS_CLEAN)

# these values come from the m4_include above
MPICH_VERSION=MPICH_VERSION_m4
AC_SUBST([MPICH_VERSION])
MPICH_RELEASE_DATE="MPICH_RELEASE_DATE_m4"
AC_SUBST([MPICH_RELEASE_DATE])
libmpi_so_version="libmpi_so_version_m4"
AC_SUBST([libmpi_so_version])


if test -z "$MPICH_VERSION" ; then
    AC_MSG_ERROR([MPICH_VERSION is empty, check maint/version.m4 for errors])
fi
# Produce a numeric version assuming the following format:
# Version: [MAJ].[MIN].[REV][EXT][EXT_NUMBER]
# Example: 1.0.7rc1 has
#          MAJ = 1
#          MIN = 0
#          REV = 7
#          EXT = rc
#          EXT_NUMBER = 1
#
# Converting to numeric version will convert EXT to a format number:
#          ALPHA (a) = 0
#          BETA (b)  = 1
#          RC (rc)   = 2
#          PATCH (p) = 3
# Regular releases are treated as patch 0
#
# Numeric version will have 1 digit for MAJ, 2 digits for MIN,
# 2 digits for REV, 1 digit for EXT and 2 digits for EXT_NUMBER.
changequote(<<,>>)
V1=`expr $MPICH_VERSION : '\([0-9]*\)\.[0-9]*\.*[0-9]*[a-zA-Z]*[0-9]*'`
V2=`expr $MPICH_VERSION : '[0-9]*\.\([0-9]*\)\.*[0-9]*[a-zA-Z]*[0-9]*'`
V3=`expr $MPICH_VERSION : '[0-9]*\.[0-9]*\.*\([0-9]*\)[a-zA-Z]*[0-9]*'`
V4=`expr $MPICH_VERSION : '[0-9]*\.[0-9]*\.*[0-9]*\([a-zA-Z]*\)[0-9]*'`
V5=`expr $MPICH_VERSION : '[0-9]*\.[0-9]*\.*[0-9]*[a-zA-Z]*\([0-9]*\)'`
changequote([,])

if test "$V2" -le 9 ; then V2=0$V2 ; fi
if test "$V3" = "" ; then V3=0; fi
if test "$V3" -le 9 ; then V3=0$V3 ; fi
if test "$V4" = "a" ; then
    V4=0
elif test "$V4" = "b" ; then
    V4=1
elif test "$V4" = "rc" ; then
    V4=2
elif test "$V4" = "" ; then
    V4=3
    V5=0
elif test "$V4" = "p" ; then
    V4=3
fi
if test "$V5" -le 9 ; then V5=0$V5 ; fi

MPICH_NUMVERSION=`expr $V1$V2$V3$V4$V5 + 0`
AC_SUBST(MPICH_NUMVERSION)

AC_ARG_WITH(custom-version-string,
            AC_HELP_STRING([--with-custom-version-string], [Adds a user-specified value to the output of the mpichversion executable]),,with_custom_version_string="")
MPICH_CUSTOM_STRING=$with_custom_version_string
AC_SUBST(MPICH_CUSTOM_STRING)

# ABIVERSION is the name used by simplemake, so we reassign the
# libmpi_so_version number to it
ABIVERSION=${libmpi_so_version}
export ABIVERSION
export libmpi_so_version
AC_SUBST(ABIVERSION)

# Print out the configure options
CONFIGURE_ARGUMENTS="$ac_configure_args"
AC_SUBST(CONFIGURE_ARGUMENTS)
if test -n "$ac_configure_args" ; then
    echo "Configuring MPICH version $MPICH_VERSION with $ac_configure_args"
else
    echo "Configuring MPICH version $MPICH_VERSION"
fi

# Add the information on the system:
echo "Running on system: `uname -a`"

dnl Definitions will be placed in this file rather than in the DEFS variable
AC_CONFIG_HEADER(src/include/mpichconf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICHCONF_H_INCLUDED
#define MPICHCONF_H_INCLUDED
])

# We use an #include strategy here because all of the alternative strategies for
# quashing these variables have various drawbacks.  The alternatives are listed
# here to avoid rediscovery of these problems by someone else in the future:
#
# 1) Strategy: Rewrite mpichconf.h.in with sed at autogen.sh time.
#    Problem: Automatic remaking of config.status and friends will re-run
#             autoheader and blow away our sed changes without an opportunity to
#             patch the resulting file again.
# 2) Strategy: Add literal "#undef PACKAGE" lines to the AH_BOTTOM text.
#    Problem: These lines get rewritten by config.status to be "#define" lines,
#             so the intended quashing never actually occurs.
# 3) Strategy: Use AC_CONFIG_COMMANDS to run a sed rewrite command on
#              mpichconf.h at config.status time.
#    Problem: Causes mpichconf.h to always be rewritten, first by the normal
#             config.status commands and then by sed.  This can cause
#             unnecessary remaking of object files since nearly every C source
#             file includes this header (see the Autoconf Manual, "Automatic
#             Remaking")
#
# The only other plausible strategy would seem to be rewriting config.status
# itself via AC_CONFIG_COMMANDS_POST, but that seems error prone.  The best
# solution would be to stop all subconfigures from including config.h headers
# from other packages.  Then all of this nonsense can be eliminated.
# [goodell@ 2011-08-26]
AH_BOTTOM([
/* Include nopackage.h to undef autoconf-defined macros that cause conflicts in
 * subpackages.  This should not be necessary, but some packages are too
 * tightly intertwined right now (such as ROMIO and the MPICH core) */
#include "nopackage.h"

#endif /* !defined(MPICHCONF_H_INCLUDED) */
])

dnl Set the directory that contains support scripts such as install-sh and
dnl config.guess
AC_CONFIG_AUX_DIR(confdb)
dnl our macro dir is the same as our aux dir
AC_CONFIG_MACRO_DIR([confdb])

# Set the FROM_MPICH variable to tell subconfigures that they are
# built from within MPICH
FROM_MPICH=yes
export FROM_MPICH

# Save a copy of precious flags as USER_* before any of these flags
# are being modified by configure tests.
PAC_PREFIX_ALL_FLAGS(USER)

# WRAPPER_xFLAGS are used by mpicc and friends.
#
# WRAPPER_CFLAGS and other compile flags are used for compile options
# that are added by MPICH, but should be used by applications (such
# as include paths).
#
# All libraries that are detected by MPICH as needed for some of its
# functionality (such as -lpthread) should be added to WRAPPER_LIBS
# so executables built within MPICH use them.  If inter-library
# dependencies are not supported on the platform, these libraries are
# added to the MPICH wrappers (mpicc and friends) as well.
PAC_PREFIX_ALL_FLAGS(WRAPPER)

# MPICH_MPIx_FLAGS are used by mpicc and friends.  They are like
# WRAPPER flags, but these are provided by the user.
AC_SUBST(MPICH_MPICC_CPPFLAGS)
AC_SUBST(MPICH_MPICC_CFLAGS)
AC_SUBST(MPICH_MPICC_LDFLAGS)
AC_SUBST(MPICH_MPICC_LIBS)

AC_SUBST(MPICH_MPICXX_CPPFLAGS)
AC_SUBST(MPICH_MPICXX_CXXFLAGS)
AC_SUBST(MPICH_MPICXX_LDFLAGS)
AC_SUBST(MPICH_MPICXX_LIBS)

AC_SUBST(MPICH_MPIF77_CPPFLAGS)
AC_SUBST(MPICH_MPIF77_FFLAGS)
AC_SUBST(MPICH_MPIF77_LDFLAGS)
AC_SUBST(MPICH_MPIF77_LIBS)

AC_SUBST(MPICH_MPIFORT_CPPFLAGS)
AC_SUBST(MPICH_MPIFORT_FCFLAGS)
AC_SUBST(MPICH_MPIFORT_LDFLAGS)
AC_SUBST(MPICH_MPIFORT_LIBS)

# Add MPICHLIB_* to the appropriate flags
AC_ARG_VAR(MPICHLIB_CFLAGS,
	[extra CFLAGS used in building MPICH libraries])
AC_ARG_VAR(MPICHLIB_CPPFLAGS,
	[extra CPPFLAGS used in building MPICH libraries])
AC_ARG_VAR(MPICHLIB_CXXFLAGS,
	[extra CXXFLAGS used in building MPICH libraries])
AC_ARG_VAR(MPICHLIB_FFLAGS,
	[extra FFLAGS used in building MPICH libraries])
AC_ARG_VAR(MPICHLIB_FCFLAGS,
	[extra FCFLAGS used in building MPICH libraries])
CFLAGS="$CFLAGS $MPICHLIB_CFLAGS"
CPPFLAGS="$CPPFLAGS $MPICHLIB_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $MPICHLIB_CXXFLAGS"
FFLAGS="$FFLAGS $MPICHLIB_FFLAGS"
FCFLAGS="$FCFLAGS $MPICHLIB_FCFLAGS"

dnl include all subsystem m4 fragments now that the core autoconf functionality
dnl has been setup.  No fragment should do anything except define
dnl PAC_SUBCFG_{PREREQ,BODY} macros which will be expanded later as
dnl appropriate
# begin subsys includes
m4_include([subsys_include.m4])
# end subsys includes

dnl ----------------------------------------------------------------------------
dnl setup top-level argument handling
AC_ARG_ENABLE(echo,
	AC_HELP_STRING([--enable-echo], [Turn on strong echoing. The default is enable=no.]),
	set -x)

AC_ARG_ENABLE(error-checking,
[  --enable-error-checking=level
      Control the amount of error checking.
        no        - no error checking
        runtime   - error checking controllable at runtime through environment
                    variables
        all       - error checking always enabled (default)
],,enable_error_checking=all)

AC_ARG_ENABLE(error-messages,
[  --enable-error-messages=level - Control the amount of detail in error messages.
        all       - Maximum amount of information
        generic   - Only generic messages (no information about the specific
                    instance)
        class     - One message per MPI error class
        none      - No messages
],,enable_error_messages=all)

AC_ARG_ENABLE(tag-error-bits,
[  --enable-tag-error-bits=yes|no - Control whether bits are taken from the user tag for error handling.
        yes       - Two bits are taken from the user tag to support error propagation.
        no        - No bits are taken from the user tag (this could cause deadlock if an error is detected during a collective).
],,enable_tag_error_bits=yes)

AC_ARG_ENABLE(timing,
[  --enable-timing=level - Control the amount of timing information
                           collected by the MPICH implementation.
        none    - Collect no data (default)
        all     - Collect lots of data
        runtime - Runtime control of data collected
],,enable_timing=default)

AC_ARG_ENABLE(g,
[  --enable-g=option - Control the level of debugging support in the
                       MPICH implementation.  "option" is a list of comma
                       separated names including.  Default is "most".
        none     - No debugging
        handle   - Trace handle operations
        handlealloc - Trace handle allocations
        dbg      - Add compiler flag, -g, to all internal
                   compiler flags, i.e. MPICHLIB_CFLAGS, MPICHLIB_CXXFLAGS,
                   MPICHLIB_FFLAGS, and MPICHLIB_FCFLAGS.
        debug    - Synonym for dbg
        mem      - Memory usage tracing
        meminit  - Preinitialize memory associated structures and unions to
                   eliminate access warnings from programs like valgrind
        memarena - Check for overwrite errors in memory allocation arena
        mutex    - Enable error checking on pthread mutexes
        mutexnesting - Check for non-nesting of mutexes
        most     - Most of the above options, excluding some with severe
                   performance impacts.  Recommended for typical development.
        yes      - synonym for "most" (*not* "all")
        all      - All of the above choices
],,enable_g=none)

AC_ARG_ENABLE([mpit-pvars],
[  --enable-mpit-pvars=list - Selectively enable MPI_T performance variables in
                      modules. list is a comma-separated module names,
                      including (Default is "none"):
        none     - No performance info recorded
        recvq    - All message queue-related
        nem      - All nemesis-related
        rma      - All rma-related
        all      - All variables above
],[],[enable_mpit_pvars=none])

AC_ARG_ENABLE(fast,
[  --enable-fast=option - Control the level of fast execution in the
                         MPICH implementation.  option is a list of
                         comma separated names including
        O<n>     - Appends default optimization flags, -O<n>, to all internal
                   compiler flags, i.e. MPICHLIB_CFLAGS, MPICHLIB_CXXFLAGS,
                   MPICHLIB_FFLAGS, and MPICHLIB_FCFLAGS. (default is -O2)
        ndebug   - Appends -DNDEBUG to MPICHLIB_CFLAGS.
        all|yes  - "O2" and "ndebug" are enabled
        none     - None of above options, i.e. --disable-fast
],,enable_fast=O2)

AC_ARG_ENABLE(interlib-deps,
	[AC_HELP_STRING([--enable-interlib-deps - Enable interlibrary dependencies])],,enable_interlib_deps=yes)

AC_ARG_ENABLE(check-compiler-flags,
	AC_HELP_STRING([--enable-check-compiler-flags], [enable the checks for all compiler
                       options, xxxFLAGS, MPICH_xxxFLAGS. Default is on.]),,
		       enable_check_compiler_flags=yes)

dnl We enable f77 and fc if we can find compilers for them.
dnl In addition, we check whether f77 and fc can work together.
AC_ARG_ENABLE(fortran,
[  --enable-fortran=option - Control the level of Fortran support in the MPICH implementation.
	yes|all   - Enable all available Fortran implementations (F77, F90+)
	f77       - Enable Fortran 77 support
	fc        - Enable Fortran 90 and 2008 support
	no|none   - No Fortran support
],,[enable_fortran=all])

AC_ARG_ENABLE(f77,
	AC_HELP_STRING([--enable-f77],
		[DEPRECATED: Use --enable-fortran or --disable-fortran instead]),,[enable_f77=yes])

AC_ARG_ENABLE(fc,
	AC_HELP_STRING([--enable-fc],
		[DEPRECATED: Use --enable-fortran or --disable-fortran instead]),,[enable_fc=yes])

AC_ARG_ENABLE(cxx,
	AC_HELP_STRING([--enable-cxx], [Enable C++ bindings]),,enable_cxx=yes)

AC_ARG_ENABLE(romio,
	AC_HELP_STRING([--enable-romio], [Enable ROMIO MPI I/O implementation]),,
	enable_romio=yes)

AC_ARG_ENABLE(debuginfo,
	AC_HELP_STRING([--enable-debuginfo], [Enable support for debuggers]),,
	enable_debuginfo=no)


## Enable creation of libtool-style versioning or no versioning
AC_ARG_ENABLE(versioning,
        [AC_HELP_STRING([--enable-versioning],[Enable library versioning])],,
        [enable_versioning=yes])

if test "$enable_versioning" = "yes" ; then
   ABIVERSIONFLAGS="-version-info \$(ABIVERSION)"
else
   ABIVERSIONFLAGS="-avoid-version"
fi
export ABIVERSIONFLAGS
AC_SUBST(ABIVERSIONFLAGS)


dnl The environment variable MPICH_DEBUGLIBNAME may be used to
dnl override the default name of the library that the debugger will
dnl load to access the MPICH internal data structures.

dnl "default" is a special device that allows MPICH to choose one
dnl based on the environment.
AC_ARG_WITH(device,
	AC_HELP_STRING([--with-device=name], [Specify the communication device for MPICH]),,
	with_device=default)

AC_ARG_WITH(pmi,
	AC_HELP_STRING([--with-pmi=name], [Specify the pmi interface for MPICH]),,
	with_pmi=default)

AC_ARG_WITH(pm,
	AC_HELP_STRING([--with-pm=name],
		[Specify the process manager for MPICH.  "no" or "none" are
                 valid values.  Multiple process managers may be specified as
                 long as they all use the same pmi interface by separating them
                 with colons.  The mpiexec for the first named process manager
                 will be installed.  Example: "--with-pm=hydra:gforker"
                 builds the two process managers hydra, and gforker;
                 only the mpiexec from hydra is installed into the bin
                 directory.]),,with_pm=default)

AC_ARG_WITH(logging,
	AC_HELP_STRING([--with-logging=name], [Specify the logging library for MPICH]),
	[if test -z "$withval" ; then with_logging=rlog ; fi],with_logging=none)

AC_ARG_ENABLE(threads,
[  --enable-threads=level - Control the level of thread support in the
                           MPICH implementation.  The following levels
                           are supported.
        single          - No threads (MPI_THREAD_SINGLE)
        funneled        - Only the main thread calls MPI (MPI_THREAD_FUNNELED)
        serialized      - User serializes calls to MPI (MPI_THREAD_SERIALIZED)
        multiple        - Fully multi-threaded (MPI_THREAD_MULTIPLE)
        runtime         - Alias to "multiple"

        See also the --enable-thread-cs option for controlling the granularity of
        the concurrency inside of the library
],,enable_threads=default)

AC_ARG_ENABLE(thread-cs,
	AC_HELP_STRING([--enable-thread-cs=type],
			[Choose the method used for critical sections
                         and other atomic updates when multiple
                         threads are present.  Values may be global
                         (default), per-object, per-vci, lock-free]),,enable_thread_cs=global)

AC_ARG_ENABLE(mdta,
    AC_HELP_STRING([--enable-mdta],
            [Optimize global granularity using message-driven thread activation.
                         Only available for ch4 device with --enable-thread-cs=global]),
    AC_DEFINE(MPICH_THREAD_USE_MDTA,1,[Define to enable message-driven thread activation]))

AC_ARG_ENABLE(refcount,
	AC_HELP_STRING([--enable-refcount=type],
			[Choose the method for ensuring atomic updates
                         to the reference counts for MPI objects.
                         Values may be lock-free or none.  The
                         default depends on the thread-cs choice; for
                         global it is none (because none is required),
                         for per-object, per-vci, and lock-free, lock-free]),,
                         enable_refcount=default)

AC_ARG_ENABLE(mutex-timing,
	AC_HELP_STRING([--enable-mutex-timing], [calculate the time spent waiting on mutexes]),
	AC_DEFINE(MPIU_MUTEX_WAIT_TIME,1,[Define to enable timing mutexes]))

AC_ARG_ENABLE([predefined-refcount],
	AS_HELP_STRING([--enable-predefined-refcount],
                       [control whether predefined objects like
		       MPI_COMM_WORLD are reference counted (default
		       depends on --enable-thread-cs choice)]),[],
              [enable_predefined_refcount=default])

AC_ARG_ENABLE(weak-symbols,
	AC_HELP_STRING([--enable-weak-symbols],
			[Use weak symbols to implement PMPI routines (default)]),,
		enable_weak_symbols=yes)

AC_ARG_ENABLE([two-level-namespace],
              [AS_HELP_STRING([--enable-two-level-namespace],
                              [(Darwin only) Build shared libraries and programs
                               built with the mpicc/mpifort/etc. compiler
                               wrappers with '-Wl,-commons,use_dylibs' and
                               without '-Wl,-flat_namespace'.  This may make the
                               MPICH installation and MPI programs more
                               compatible with other libraries.  Only enable
                               this option if you really know what these linker
                               options imply.])],
              [],
              [enable_two_level_namespace=no])

AC_ARG_ENABLE(multi-aliases,
	AC_HELP_STRING([--enable-multi-aliases],
		[Multiple aliasing to support multiple fortran compilers (default)]),,
		enable_multi_aliases=yes)

AC_ARG_WITH([wrapper-dl-type],
            [AC_HELP_STRING([--enable-wrapper-dl-type],
                            [Dynamic loading model for alternate MPI
                             libraries, used when programs are linked
                             by mpicc compiler wrappers.  This only
                             applies when shared libraries are built.
                             The default is "runpath"; use
                             --with-wrapper-dl-type=rpath to force
                             rpath; use --with-wrapper-dl-type=none to
                             find shared libraries according to the
                             rules for your system (e.g., in
                             LD_LIBRARY_PATH)])],
              [],[with_wrapper_dl_type=runpath])
AC_SUBST([with_wrapper_dl_type])

AC_ARG_ENABLE([long-double],
              [AC_HELP_STRING([--disable-long-double],
                              [Pass --disable-long-double to prevent the MPI
                               library from supporting the C "long double" type,
                               even if the C compiler supports it.  "long
                               double" support is enabled by default, provided
                               the compiler supports it.])],
              [],
              [enable_long_double=yes])

AC_ARG_WITH(cross,
	AC_HELP_STRING([--with-cross=file],
		[Specify the values of variables that configure cannot
                 determine in a cross-compilation environment]),,
		 with_cross=$MPID_DEFAULT_CROSS_FILE)

AC_ARG_WITH(namepublisher,
[  --with-namepublisher=name   Choose the system that will support
                              MPI_PUBLISH_NAME and MPI_LOOKUP_NAME.  Options
                              include
                                   pmi (default)
			           file[:directory] (optional directory)
                                   no (no service available)],,with_namepublisher=default)
AC_ARG_WITH(name-publisher,
    [],
    with_namepublisher=$with_name_publisher,)

AC_ARG_ENABLE(dbg-nolocal, AC_HELP_STRING([--enable-dbg-nolocal], [enables debugging mode where shared-memory communication is disabled]),
    AC_DEFINE(ENABLED_NO_LOCAL, 1, [Define to disable shared-memory communication for debugging]))

AC_ARG_ENABLE(dbg-localoddeven, AC_HELP_STRING([--enable-dbg-localoddeven], [enables debugging mode where shared-memory communication is enabled only between even processes or odd processes on a node]),
    AC_DEFINE(ENABLED_ODD_EVEN_CLIQUES, 1, [Define to enable debugging mode where shared-memory communication is done only between even procs or odd procs]))

AC_CANONICAL_TARGET

# Find a C compiler.
# We also need to do this before the F77 and FC test to ensure that we
# find the C preprocessor reliably.
AC_PROG_CC
AM_PROG_CC_C_O dnl needed for automake "silent-rules"
PAC_PUSH_FLAG([CFLAGS])
AC_PROG_CPP
# Bug in autoconf.  Restore cross settings
if test "$pac_cross_compiling" = "yes" -a "$ac_cv_prog_cc_cross" = "no" ; then
    AC_MSG_RESULT([Resetting cross compilation to yes])
    cross_compiling=yes
    ac_cv_prog_cc_cross=yes
    ac_cv_prog_f77_cross=yes
    ac_cv_prog_fc_cross=yes
    ac_cv_prog_cxx_cross=yes
fi
PAC_POP_FLAG([CFLAGS])

# also needed by hwloc in embedded mode, must also come early for expansion
# ordering reasons
AC_USE_SYSTEM_EXTENSIONS

dnl now that autoconf and core compilers are setup, init automake and libtool
dnl
dnl We would like to pass -Werror, but we are cheating in the "examples/"
dnl directory and overriding the user-flags like CFLAGS, which automake-1.12
dnl warns about.  Long-term we may need to use a hand-written Makefile.in or
dnl something else in this special dir.
AM_INIT_AUTOMAKE([-Wall -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects])
AM_MAINTAINER_MODE([enable])

AM_PROG_AR

LT_INIT()
# Non-verbose make by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Disable rpath/runpath-related linkflags in the compiler wrappers if
# shared libraries are disabled, since rpath/runpath makes no sense in
# the context of static libraries.
if test "X$enable_shared" = "Xno" ; then
    with_wrapper_dl_type=no
fi

INTERLIB_DEPS=yes
# We conservatively disable interlibrary dependencies if the libtool
# support model is anything different from "pass_all"
if test "X$enable_shared" = "Xno" -o "$deplibs_check_method" != "pass_all" -o "$enable_interlib_deps" = "no" ; then
    INTERLIB_DEPS=no
fi
export INTERLIB_DEPS
AC_SUBST(INTERLIB_DEPS)

dnl AC_PROG_{CXX,F77,FC} must come early in configure.ac in order to avoid some
dnl esoteric autoconf macro expansion errors
dnl
dnl Also, DO NOT attempt to place shell conditionals (either manually or via
dnl AS_IF) around these macros in an effort to save configure time.  It will
dnl lead to weird AM_CONDITIONAL errors and potentially other problems.

# Before attempting to find valid compilers, set the corresponding precious
# shell variable to "no" for any languages that have been disabled by the user
# with "--disable-LANG".  Libtool understands this as a request to disable
# support for this language. This should save a bit of configure time and also
# prevent user complaints like ticket #1570.
AS_IF([test "x$enable_f77" = "xno"],[F77=no])
AS_IF([test "x$enable_fc"  = "xno"],[FC=no])
AS_IF([test "x$enable_cxx" = "xno"],[CXX=no])

# suppress default "-g -O2" from AC_PROG_CXX
: ${CXXFLAGS=""}
AC_PROG_CXX([PAC_CXX_SEARCH_LIST])

# suppress default "-g -O2" from AC_PROG_FC
: ${FCFLAGS=""}
AC_PROG_FC([PAC_FC_SEARCH_LIST])

save_IFS="$IFS"
IFS=","
enable_f77=no
enable_fc=no
for option in $enable_fortran ; do
    case "$option" in
        yes|all)
		enable_f77=yes
		enable_fc=yes
		;;
        no|none)
		;;
        f77)
		enable_f77=yes
		;;
        fc)
		enable_fc=yes
		;;
        *)
		IFS="$save_IFS"
		AC_MSG_WARN([Unknown value $option for --enable-fortran])
		IFS=","
		;;
    esac
done
IFS="$save_IFS"

if test "$enable_f77" = "no" ; then
   if test "$enable_fc" = "yes" ; then
      AC_MSG_ERROR([Fortran 90 support requires enabling Fortran 77])
   fi
fi

if test ! -z "$FC" -a -z "$F77" ; then
   F77=$FC
   if test ! -z "$FCFLAGS" -a -z "$FFLAGS" ; then
      FFLAGS=$FCFLAGS
   fi
fi

AM_CONDITIONAL([INSTALL_MPIF77],[test "$F77" != "$FC" -a "$FFLAGS" != "$FCFLAGS"])

# This needs to come after we've potentially set F77=$FC. Otherwise, we could
# override the user's Fortran compiler selection when only specifying FC at configure
# time, as is allowed.
# suppress default "-g -O2" from AC_PROG_F77
: ${FFLAGS=""}
AC_PROG_F77([PAC_F77_SEARCH_LIST])

# compute canonical system types
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
# TARGET not needed, MPICH isn't a compiler

# Enable better caching control
PAC_ARG_CACHING

# Set CFLAGS for enable strict if necessary.  Do this *first* because
# it may influence the output of the other tests
PAC_ARG_STRICT

# -----------------------------------------------------------------------------
# First check that we have a clean build if we are doing a VPATH build
PAC_VPATH_CHECK(src/include/mpi.h src/env/mpicc,lib)

# ----------------------------------------------------------------------------
# This test is complicated by the fact that top_srcdir is not set until
# the very end of configure.  Instead, we get it ourselves
if test -z "$top_srcdir" ; then
   use_top_srcdir=$srcdir
else
   use_top_srcdir=$top_srcdir
fi
if test -z "$master_top_srcdir" ; then
    # This needs to be an absolute pathname
    case "$use_top_srcdir" in
    /*) ;;
    *)
        use_top_srcdir=`(cd $use_top_srcdir && pwd)`
	;;
    esac
    master_top_srcdir=$use_top_srcdir
fi
# Get the directory that we're running in...
if test -z "$master_top_builddir" ; then
   master_top_builddir="`pwd`"
fi
AC_SUBST(master_top_builddir)
AC_SUBST(master_top_srcdir)
export master_top_builddir
export master_top_srcdir

if test -z "$with_cross"; then
  if test -f "$master_top_srcdir/src/cross/$host_alias"; then
    with_cross="$master_top_srcdir/src/cross/$host_alias"
  else
    with_cross=no
  fi
fi
if test "$with_cross" != "no"; then
  AC_MSG_NOTICE([Using cross file: $with_cross])
fi

# ----------------------------------------------------------------------------
dnl Export important "precious" variables so that any directories configured via
dnl PAC_CONFIG_SUBDIR will agree with the top-level configure about these
dnl critical variables (esp. compiler selection).  These exports should come
dnl before any subconfigures in this script.
dnl
dnl This list is arguably incomplete, and should possibly be automatically
dnl generated from "$ac_precious_vars" using code similar to the implementation
dnl of PAC_CONFIG_SUBDIR.
dnl
dnl To be clear, without these exports any variable values determined by this
dnl configure script will not be seen by child scripts.  Instead they will dnl
dnl receive the only the original inherited environment and configure args used
dnl when this configure script was invoked.
export CC
export CFLAGS
export CPPFLAGS
export CXX
export CXXFLAGS
export F77
export FC
export FCFLAGS
export FFLAGS
export LDFLAGS
export LIBS
export MPILIBNAME
export PMPILIBNAME
export OPALIBNAME
export MPLLIBNAME
# ----------------------------------------------------------------------------
# with-device
if test "$with_device" = "default" ; then
    # Pick the device.  For now, always choose ch3
    with_device=ch3
fi
# Extract the device name from any options
# Allow the device to specify a directory; if no directory, use the
# included directories
#
DEVICE=$with_device
AC_SUBST(DEVICE)

device_name=`echo $with_device | sed -e 's/:.*$//'`
changequote(<<,>>)
device_args=`echo $with_device | sed -e 's/^[^:]*//' -e 's/^://'`
changequote([,])

devicedir=$use_top_srcdir/src/mpid/$device_name
devicereldir=src/mpid/$device_name
export device_name
export device_args
export devicedir

# expand all of the prereq macros in the correct order
m4_map([PAC_SUBCFG_DO_PREREQ], [PAC_SUBCFG_MODULE_LIST])

# ----------------------------------------------------------------------------
# Set default library names if names haven't already been provided
AC_ARG_VAR([MPILIBNAME],[can be used to override the name of the MPI library (default: "mpi")])
AC_ARG_VAR([PMPILIBNAME],[can be used to override the name of the MPI profiling library (default: "p$MPILIBNAME")])
AC_ARG_VAR([MPICXXLIBNAME],[can be used to override the name of the MPI C++ library (default: "${MPILIBNAME}cxx")])
AC_ARG_VAR([MPIFCLIBNAME],[can be used to override the name of the MPI fortran library (default: "${MPILIBNAME}fort")])
MPILIBNAME=${MPILIBNAME:-"mpi"}
PMPILIBNAME_set=no
if test -n "$PMPILIBNAME" ; then
   PMPILIBNAME_set=yes
fi
PMPILIBNAME=${PMPILIBNAME:-"p$MPILIBNAME"}
# Note that the name for this library may be updated after we check for
# enable_shmem
# Fortran names are set later.
# We use a different library for the C++ wrappers to avoid problems when
# creating shared libraries
if test -z "$MPICXXLIBNAME" ; then MPICXXLIBNAME="${MPILIBNAME}cxx" ; fi
if test -z "$MPIFCLIBNAME" ; then MPIFCLIBNAME="${MPILIBNAME}fort" ; fi
export MPICXXLIBNAME
export MPIFCLIBNAME
AC_SUBST(MPICXXLIBNAME)
AC_SUBST(MPIFCLIBNAME)

# We'll set FORTRAN_BINDING to 1 if we support Fortran
FORTRAN_BINDING=0

# enable-fast
# strip off multiple options, separated by commas
save_IFS="$IFS"
IFS=","
for option in $enable_fast ; do
    case "$option" in
        O*)
        enable_fast_opts=$option
        ;;
        ndebug)
        enable_fast_ndebug=yes
        ;;
        all|yes)
        enable_fast_ndebug=yes
        enable_fast_opts=O2
        ;;
        none|no)
        enable_fast_ndebug=no
        enable_fast_opts=O0
        ;;
        *)
	IFS="$save_IFS"
        AC_MSG_WARN([Unknown value $option for --enable-fast])
	IFS=","
        ;;
    esac
done
IFS="$save_IFS"

if test -n "$enable_fast_opts" ; then
   # Allows O<n> where <n> can be [0-9] or ' '.
   opt_flags=`echo $enable_fast_opts | sed -e 's%\(O[0-9] \)%\1%g'`
   if test -n "$opt_flags" ; then
      MPI_DEFAULT_COPTS="-$enable_fast_opts"
      MPI_DEFAULT_CXXOPTS="-$enable_fast_opts"
      MPI_DEFAULT_FOPTS="-$enable_fast_opts"
      MPI_DEFAULT_FCOPTS="-$enable_fast_opts"
   else
      AC_MSG_WARN([Unknown value $enable_fast_opts for --enable-fast])
   fi
fi

if test "$enable_fast_ndebug" = "yes" ; then
    CFLAGS="$CFLAGS -DNDEBUG -DNVALGRIND"
    CXXFLAGS="$CXXFLAGS -DNDEBUG -DNVALGRIND"
    # MPICH does NOT assume any preprocessing support from the Fortran compiler,
    # so no Fortran files contain any preprocessing statements.
    # Don't set FFLAGS or FCFLAGS with any -D.
fi

# error-checking
# Change default into the specific value of the default
if test "$enable_error_checking" = "yes" ; then
   enable_error_checking=all
fi
# mpir_ext.h needs the variable HAVE_ERROR_CHECKING to have the value 0 or 1
HAVE_ERROR_CHECKING=0
case "$enable_error_checking" in
    no)
    # if error checking has been disabled, then automatically disable the error
    # checking tests in the test suite
    ac_configure_args="${ac_configure_args} --disable-checkerrors"
    ;;
    all|runtime)
    error_checking_kind=`echo $enable_error_checking | \
    tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    error_checking_kind=MPID_ERROR_LEVEL_$error_checking_kind
    AC_DEFINE_UNQUOTED(HAVE_ERROR_CHECKING,$error_checking_kind,[Define to enable error checking])
    HAVE_ERROR_CHECKING=1
    ;;
    *)
    AC_MSG_WARN([Unknown value $enable_error_checking for enable-error-checking])
    ;;
esac
# permit @HAVE_ERROR_CHECKING@ substitution in mpir_ext.h
AC_SUBST([HAVE_ERROR_CHECKING])

# error-messages
case "$enable_error_messages" in
    no|none)
        error_message_kind="MPICH_ERROR_MSG__NONE"
    ;;
    all|yes)
	error_message_kind="MPICH_ERROR_MSG__ALL"
    ;;
    generic)
	error_message_kind="MPICH_ERROR_MSG__GENERIC"
    ;;
    class)
	error_message_kind="MPICH_ERROR_MSG__CLASS"
    ;;
    *)
    AC_MSG_WARN([Unknown value $enable_error_messages for enable-error-messages])
    ;;
esac
AC_DEFINE_UNQUOTED(MPICH_ERROR_MSG_LEVEL,$error_message_kind,[define to enable error messages])

#error-tags
if test "$enable_tag_error_bits" = "yes" ; then
    AC_DEFINE([HAVE_TAG_ERROR_BITS],[1],[Define to enable tag error bits])
fi

# MPL
AC_ARG_VAR([MPLLIBNAME],[can be used to override the name of the MPL library (default: "mpl")])
MPLLIBNAME=${MPLLIBNAME:-"mpl"}
export MPLLIBNAME
AC_SUBST(MPLLIBNAME)
AC_ARG_WITH([mpl-prefix],
            [AS_HELP_STRING([[--with-mpl-prefix[=DIR]]],
                            [use the MPL library installed in DIR,
                             rather than the one included in src/mpl.  Pass
                             "embedded" to force usage of the MPL source
                             distributed with MPICH.])],
            [],dnl action-if-given
            [with_mpl_prefix=embedded]) dnl action-if-not-given
mplsrcdir=""
AC_SUBST([mplsrcdir])
mpllibdir=""
AC_SUBST([mpllibdir])
mpllib=""
AC_SUBST([mpllib])
if test "$with_mpl_prefix" = "embedded" ; then
    # no need for libtool versioning when embedding MPL
    mpl_subdir_args="--disable-versioning --enable-embedded"
    PAC_CONFIG_SUBDIR_ARGS([src/mpl],[$mpl_subdir_args],[],[AC_MSG_ERROR(MPL configure failed)])
    PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpl/include], [CPPFLAGS])
    PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpl/include], [CPPFLAGS])

    mplsrcdir="src/mpl"
    mpllib="src/mpl/lib${MPLLIBNAME}.la"
else
    # The user specified an already-installed MPL; just sanity check, don't
    # subconfigure it
    AS_IF([test -s "${with_mpl_prefix}/include/mplconfig.h"],
          [:],[AC_MSG_ERROR([the MPL installation in "${with_mpl_prefix}" appears broken])])
    PAC_APPEND_FLAG([-I${with_mpl_prefix}/include],[CPPFLAGS])
    PAC_PREPEND_FLAG([-l${MPLLIBNAME}],[WRAPPER_LIBS])
    PAC_APPEND_FLAG([-L${with_mpl_prefix}/lib],[WRAPPER_LDFLAGS])
    mpllibdir="-L${with_mpl_prefix}/lib"
fi

# OpenPA
AC_ARG_VAR([OPALIBNAME],[can be used to override the name of the OpenPA library (default: "opa")])
OPALIBNAME=${OPALIBNAME:-"opa"}
export OPALIBNAME
AC_SUBST(OPALIBNAME)
AC_ARG_WITH([openpa-prefix],
            [AS_HELP_STRING([[--with-openpa-prefix[=DIR]]],
                            [use the OpenPA atomics library installed in DIR,
                             rather than the one included in src/openpa.  Pass
                             "embedded" to force usage of the OpenPA source
                             distributed with MPICH.])],
            [],
            [# see if OPA is already installed on the system
             PAC_PUSH_FLAG([LIBS])
             PAC_PREPEND_FLAG([-l${OPALIBNAME}],[LIBS])
             AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
#include "opa_primitives.h"
],[
OPA_int_t i;
OPA_store_int(i,10);
OPA_fetch_and_incr_int(&i,5);
])dnl
                             ],
                            [with_openpa_prefix=system],[with_openpa_prefix=embedded])
             PAC_POP_FLAG([LIBS])
             ])

opasrcdir=""
AC_SUBST([opasrcdir])
opalibdir=""
AC_SUBST([opalibdir])
opalib=""
AC_SUBST([opalib])

if test "$with_openpa_prefix" = "embedded" ; then
    if test -e "${use_top_srcdir}/src/openpa" ; then
        opasrcdir="src/openpa"
        opalib="src/openpa/src/lib${OPALIBNAME}.la"
        PAC_APPEND_FLAG([-I${use_top_srcdir}/src/openpa/src],[CPPFLAGS])
        PAC_APPEND_FLAG([-I${master_top_builddir}/src/openpa/src],[CPPFLAGS])

        # OPA defaults to "auto", but in MPICH we want "auto_allow_emulation" to
        # easily permit using channels like ch3:sock that don't care about atomics
        AC_ARG_WITH([atomic-primitives],
                    [AS_HELP_STRING([--with-atomic-primitives],
                                    [Force OPA to use a specific atomic primitives
                                     implementation.  See the src/openpa directory
                                     for more info.])],
                    [],[with_atomic_primitives=not_specified])
        # no need for libtool versioning when embedding OPA
        opa_subdir_args="--disable-versioning --enable-embedded"
        if test "$with_atomic_primitives" = "not_specified" ; then
            PAC_APPEND_FLAG([--with-atomic-primitives=auto_allow_emulation], [opa_subdir_args])
        fi
        PAC_CONFIG_SUBDIR_ARGS([src/openpa],[$opa_subdir_args],[],[AC_MSG_ERROR([OpenPA configure failed])])
    else
        AC_MSG_WARN([Attempted to use the embedded OpenPA source tree in "src/openpa", but it is missing.  Configuration or compilation may fail later.])
    fi
elif test "$with_openpa_prefix" = "system" ; then
    PAC_PREPEND_FLAG([-l${OPALIBNAME}],[WRAPPER_LIBS])
elif test "$with_openpa_prefix" = "no" ; then
    # The user doesn't want to use OPA.  This may or may not cause MPICH to
    # fail to configure/build, depending on many other factors.
    :
else
    # The user specified an already-installed OPA; just sanity check, don't
    # subconfigure it
    AS_IF([test -s "${with_openpa_prefix}/include/opa_primitives.h" -a -s "${with_openpa_prefix}/include/opa_config.h"],
          [:],[AC_MSG_ERROR([the OpenPA installation in "${with_openpa_prefix}" appears broken])])
    PAC_APPEND_FLAG([-I${with_openpa_prefix}/include],[CPPFLAGS])
    PAC_PREPEND_FLAG([-l${OPALIBNAME}],[WRAPPER_LIBS])
    if test -d ${with_openpa_prefix}/lib64 ; then
        PAC_APPEND_FLAG([-L${with_openpa_prefix}/lib64],[WRAPPER_LDFLAGS])
        opalibdir="-L${with_openpa_prefix}/lib64"
    else
        opalibdir="-L${with_openpa_prefix}/lib"
    fi
    PAC_APPEND_FLAG([-L${with_openpa_prefix}/lib],[WRAPPER_LDFLAGS])
fi

# Izem

AC_ARG_ENABLE([izem],
[  --enable-izem@<:@=ARGS@:>@   Enable features from the Izem library.
                          "ARGS" is a list of comma separated features.
                          Accepted arguments are:
			  sync     - use the Izem interface for sychronization objects
                                     (locks and condition variables) instead of the MPL interface
                          queue    - use the Izem interface for atomic queue objects
                          atomic   - use the Izem interface for CPU atomics
                          yes/all  - all of the above features are enabled
                          no/none  - none of the above features are enabled],,
[enable_izem=no])

# strip off multiple options, separated by commas
save_IFS="$IFS"
IFS=","
for option in $enable_izem ; do
    case "$option" in
        sync)
            izem_sync=yes
        ;;
        queue)
            izem_queue=yes
        ;;
        atomic)
            izem_atomic=yes
        ;;
        yes|all)
            izem_sync=yes
            izem_queue=yes
            izem_atomic=yes
        ;;
        no|none)
        ;;
        *)
            AC_MSG_ERROR([Unknown value $option for enable-izem])
        ;;
    esac
done
IFS="$save_IFS"

if test "$izem_sync" = "yes" ; then
    AC_DEFINE(ENABLE_IZEM_SYNC,1,[Define to enable using Izem locks and condition variables])
fi

if test "$izem_queue" = "yes" ; then
    AC_DEFINE(ENABLE_IZEM_QUEUE,1,[Define to enable using Izem queues])
fi

if test "$izem_atomic" = "yes" ; then
    AC_DEFINE(ENABLE_IZEM_ATOMIC,1,[Define to enable using Izem CPU atomics])
fi
AM_CONDITIONAL([ENABLE_IZEM_ATOMIC], [test x$izem_atomic = xyes])

AC_ARG_VAR([ZMLIBNAME],[can be used to override the name of the Izem library (default: "zm")])
ZMLIBNAME=${ZMLIBNAME:-"zm"}
export ZMLIBNAME
AC_SUBST(ZMLIBNAME)
AC_ARG_WITH([zm-prefix],
[  --with-zm-prefix@<:@=ARG@:>@
                          specify the Izem library to use. No argument implies "yes".
                          Accepted values for ARG are:
                          yes|embedded - use the embedded Izem
                          system       - search system paths for an Izem installation
                          no           - disable Izem
                          <PATH>       - use the Izem at PATH],,
[with_zm_prefix=no])

zmsrcdir=""
AC_SUBST([zmsrcdir])
zmlibdir=""
AC_SUBST([zmlibdir])
zmlib=""
AC_SUBST([zmlib])

if test "$enable_izem" != "no" && test "$enable_izem" != "none"; then
    if test "$with_zm_prefix" = "yes" || test "$with_zm_prefix" = "embedded"; then
        if test -e "${use_top_srcdir}/src/izem" ; then
            zm_subdir_args="--enable-embedded"
            PAC_CONFIG_SUBDIR_ARGS([src/izem],[$zm_subdir_args],[],[AC_MSG_ERROR(Izem configure failed)])
            zmsrcdir="${master_top_builddir}/src/izem"
            zmlib="${master_top_builddir}/src/izem/src/lib${ZMLIBNAME}.la"
            PAC_APPEND_FLAG([-I${use_top_srcdir}/src/izem/src/include],[CPPFLAGS])
            PAC_APPEND_FLAG([-I${master_top_builddir}/src/izem/src/include],[CPPFLAGS])
        else
            AC_MSG_WARN([Attempted to use the embedded Izem source tree in "src/izem", but it is missing.  Configuration or compilation may fail later.])
        fi
    elif test "$with_zm_prefix" = "system"; then
        # check if an Izem installation exists on this system
        PAC_PUSH_FLAG([LIBS])
        PAC_PREPEND_FLAG([-l${ZMLIBNAME}],[LIBS])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "lock/zm_ticket.h"
                                        ],
                                        [zm_ticket_t lock;
                                         zm_ticket_init(&lock);
                                         zm_ticket_acquire(&lock);
                                         zm_ticket_release(&lock);])],
                       [AC_MSG_ERROR([No usable Izem installation was found on this system])],
                       [PAC_PREPEND_FLAG([-l${ZMLIBNAME}],[WRAPPER_LIBS])])
        PAC_POP_FLAG([LIBS])
    elif test "$with_zm_prefix" = "no"; then
        AC_MSG_ERROR([Izem features were requested with --enable-izem but Izem was disabled.])
    else
        # The user specified an already-installed Izem; just sanity check, don't
        # subconfigure it

        AS_IF([test -s "${with_zm_prefix}/include/lock/zm_lock.h" -a -s "${with_zm_prefix}/include/cond/zm_cond.h"],
              [:],[AC_MSG_ERROR([Izem headers at "${with_zm_prefix}/include" are missing])])
        PAC_APPEND_FLAG([-I${with_zm_prefix}/include],[CPPFLAGS])

        PAC_PUSH_FLAG([LIBS])
        PAC_PUSH_FLAG([LDFLAGS])
        PAC_PREPEND_FLAG([-l${ZMLIBNAME}],[LIBS])
        PAC_APPEND_FLAG([-L${with_zm_prefix}/lib],[LDFLAGS])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "lock/zm_ticket.h"
                                        ],
                                        [zm_ticket_t lock;
                                         zm_ticket_init(&lock);
                                         zm_ticket_acquire(&lock);
                                         zm_ticket_release(&lock);])],
                       [],[AC_MSG_ERROR([The Izem installation at "${with_zm_prefix}" seems broken])])
        PAC_POP_FLAG([LIBS])
        PAC_POP_FLAG([LDFLAGS])
        PAC_APPEND_FLAG([-L${with_zm_prefix}/lib],[WRAPPER_LDFLAGS])
        PAC_PREPEND_FLAG([-l${ZMLIBNAME}],[WRAPPER_LIBS])
        zmlibdir="-L${with_zm_prefix}/lib"
    fi
fi


# Set NEEDSPLIB to yes if link commands need both -l$MPILIBNAME
# and -lp$MPILIBNAME.
NEEDSPLIB=yes
if test $enable_weak_symbols = yes ; then
    # Turn off weak symbols if they aren't available
    PAC_PROG_C_WEAK_SYMBOLS(,enable_weak_symbols=no)
fi
if test $enable_weak_symbols = "yes" ; then
    AC_DEFINE(USE_WEAK_SYMBOLS,1,[Define if weak symbols should be used])
    NEEDSPLIB=no
    # Check for the ability to support multiple weak symbols
    if test "$pac_cv_prog_c_weak_symbols" = "pragma weak" ; then
       PAC_PROG_C_MULTIPLE_WEAK_SYMBOLS(AC_DEFINE(HAVE_MULTIPLE_PRAGMA_WEAK,1,[Define if multiple weak symbols may be defined]))
    fi
fi
export NEEDSPLIB

AM_CONDITIONAL([BUILD_PROFILING_LIB],[test "$NEEDSPLIB" = "yes"])
PAC_CHECK_VISIBILITY
AC_SUBST(VISIBILITY_CFLAGS)
# disable visibility if building profiling library
if test "$NEEDSPLIB" = "yes" ; then
    VISIBILITY_CFLAGS=""
fi


# ----------------------------------------------------------------------------
# HWLOC
# ----------------------------------------------------------------------------
# Allow the user to override the hwloc location (from embedded to user
# path)
# HWLOC
AC_ARG_WITH([hwloc-prefix],
            [AS_HELP_STRING([[--with-hwloc-prefix[=DIR]]],
                            [use the HWLOC library installed in DIR,
                             rather than the one included in src/hwloc.  Pass
                             "embedded" to force usage of the HWLOC source
                             distributed with MPICH.])],
            [],dnl action-if-given
            [with_hwloc_prefix=embedded]) dnl action-if-not-given
hwlocsrcdir=""
AC_SUBST([hwlocsrcdir])
hwloclibdir=""
AC_SUBST([hwloclibdir])
hwloclib=""
AC_SUBST([hwloclib])

if test "$with_hwloc_prefix" = "no" ; then
   have_hwloc=no
elif test "$with_hwloc_prefix" = "embedded" ; then
   # Disable visibility when setting up hwloc
   PAC_PUSH_FLAG([enable_visibility])
   enable_visibility=no;
   HWLOC_SETUP_CORE([src/hwloc],[have_hwloc=yes],[have_hwloc=no],[1])
   # Only build hwloc in embedded mode
   if test "$have_hwloc" = "yes" ; then
      use_embedded_hwloc=yes
      hwlocsrcdir="src/hwloc"
      hwloclib="$HWLOC_EMBEDDED_LDADD"
      PAC_PREPEND_FLAG([$HWLOC_EMBEDDED_LIBS], [WRAPPER_LIBS])
      PAC_APPEND_FLAG([$HWLOC_EMBEDDED_LDFLAGS], [WRAPPER_LDFLAGS])
      # if possible, do not expose hwloc symbols in libmpi.so
      PAC_PREPEND_FLAG([$VISIBILITY_CFLAGS], [HWLOC_CFLAGS])
   fi
   PAC_POP_FLAG([enable_visibility])
else
   AC_CHECK_HEADERS([hwloc.h])
   # hwloc_topology_set_pid was added in hwloc-1.0.0, which is our
   # minimum required version
   AC_CHECK_LIB([hwloc],[hwloc_topology_set_pid])
   AC_MSG_CHECKING([if non-embedded hwloc works])
   if test "$ac_cv_header_hwloc_h" = "yes" -a "$ac_cv_lib_hwloc_hwloc_topology_set_pid" = "yes" ; then
      have_hwloc=yes
   else
      have_hwloc=no
   fi
   AC_MSG_RESULT([$have_hwloc])

   # FIXME: Disable hwloc on Cygwin for now. The hwloc package,
   # atleast as of 1.0.2, does not install correctly on Cygwin
   AS_CASE([$host], [*-*-cygwin], [have_hwloc=no])

   if test "$have_hwloc" = "yes" ; then
      hwloclib="-lhwloc"
      if test -d ${with_hwloc_prefix}/lib64 ; then
         PAC_APPEND_FLAG([-L${with_hwloc_prefix}/lib64],[WRAPPER_LDFLAGS])
         hwloclibdir="-L${with_hwloc_prefix}/lib64"
      else
	 hwloclibdir="-L${with_hwloc_prefix}/lib"
      fi
   fi
fi

if test "$have_hwloc" = "yes" ; then
   AC_DEFINE(HAVE_HWLOC,1,[Define if hwloc is available])
fi

HWLOC_DO_AM_CONDITIONALS

# ----------------------------------------------------------------------------
# NETLOC
# ----------------------------------------------------------------------------
AC_ARG_WITH([netloc-prefix],
            [AS_HELP_STRING([[--with-netloc-prefix[=DIR]]],
                            [use the NETLOC library installed in DIR]) or system to use the system library], [],
                            [with_netloc_prefix=no])

netloclibdir=""
AC_SUBST([netloclibdir])

if test "$have_hwloc" = "yes" ; then
    if test "${with_netloc_prefix}" != "no" ; then
        if test "${with_netloc_prefix}" != "system"; then
            # The user specified an already-installed Netloc; just sanity check,
            # don't subconfigure it
            AS_IF([test -s "${with_netloc_prefix}/include/netloc.h"],
              [:],[AC_MSG_ERROR([the Netloc installation in "${with_netloc_prefix}" appears broken])])
            PAC_APPEND_FLAG([-I${with_netloc_prefix}/include],[CPPFLAGS])
            PAC_APPEND_FLAG([-I${with_netloc_prefix}/include],[CFLAGS])
            PAC_PREPEND_FLAG([-lnetloc],[WRAPPER_LIBS])
            if test -d ${with_netloc_prefix}/lib64 ; then
                PAC_APPEND_FLAG([-L${with_netloc_prefix}/lib64],[WRAPPER_LDFLAGS])
                netloclibdir="-L${with_netloc_prefix}/lib64"
            else
                PAC_APPEND_FLAG([-L${with_netloc_prefix}/lib],[WRAPPER_LDFLAGS])
                netloclibdir="-L${with_netloc_prefix}/lib"
            fi
       else
           AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "netloc.h"
                                   ],
                                   [])],
                          [AC_MSG_ERROR([the Netloc installation seems to be added from system path])], [])
       fi
       AC_DEFINE(HAVE_NETLOC,1,[Define if netloc is available in either user specified path or in system path])
    fi
fi

# ----------------------------------------------------------------------------
# Threads
# ----------------------------------------------------------------------------
#
# Threads must be supported by the device.  First, set the default to
# be the highest supported by the device
# "runtime" was an old (now deprecated) option; just map it to multiple
if test "$enable_threads" = "runtime" ; then enable_threads=multiple ; fi
if test "$enable_threads" = "yes" ; then enable_threads=default ; fi
if test "$enable_threads" = "no" ; then enable_threads=single ; fi
if test "$enable_threads" = default ; then
    # XXX DJG bug is here, PREREQ is not being used right now
    if test -n "$MPID_MAX_THREAD_LEVEL" ; then
        case $MPID_MAX_THREAD_LEVEL in
            MPI_THREAD_SINGLE)     enable_threads=single ;;
            MPI_THREAD_FUNNELED)   enable_threads=funneled ;;
            MPI_THREAD_SERIALIZED) enable_threads=serialized ;;
            MPI_THREAD_MULTIPLE)   enable_threads=multiple ;;
            *) AC_MSG_ERROR([Unrecognized thread level from device $MPID_MAX_THREAD_LEVEL])
    	    ;;
        esac
    else
        enable_threads=single
    fi
fi

MPICH_THREAD_LEVEL=MPI_THREAD_FUNNELED
case "$enable_threads" in
    single)
    thread_pkg_required=no
    MPICH_THREAD_LEVEL=MPI_THREAD_SINGLE
    ;;
    funneled)
    thread_pkg_required=no
    MPICH_THREAD_LEVEL=MPI_THREAD_FUNNELED
    ;;
    serialized)
    thread_pkg_required=no
    MPICH_THREAD_LEVEL=MPI_THREAD_SERIALIZED
    ;;
    multiple)
    thread_pkg_required=yes
    MPICH_THREAD_LEVEL=MPI_THREAD_MULTIPLE
    ;;
    *)
    AC_MSG_ERROR(["$enable_threads" is not a valid value for --enable-threads])
    ;;
esac
# Check that the requested thread level is available.
threadLevelOK=yes
if test ! -z "$MPID_MAX_THREAD_LEVEL" ; then
    # Check that MPID_MAX_THREAD_LEVEL is at least as large as the
    # selected MPICH_THREAD_LEVEL
    case $MPICH_THREAD_LEVEL in
        MPI_THREAD_MULTIPLE)
	if test "$MPID_MAX_THREAD_LEVEL" != "MPI_THREAD_MULTIPLE" ; then
	    threadLevelOK=no
        fi
	;;
	MPI_THREAD_SERIALIZED)
	if test "$MPID_MAX_THREAD_LEVEL" != "MPI_THREAD_MULTIPLE" -a \
	        "$MPID_MAX_THREAD_LEVEL" != "MPI_THREAD_SERIALIZED" ; then
            threadLevelOK=no
        fi
	;;
	MPI_THREAD_FUNNELED)
        if test "$MPID_MAX_THREAD_LEVEL" = "MPI_THREAD_SINGLE" ; then
            threadLevelOK=no
        fi
	;;
	MPI_THREAD_SINGLE)
	;;
    esac
fi
if test "$threadLevelOK" != yes ; then
   AC_MSG_ERROR([The device $with_device does not support $MPICH_THREAD_LEVEL])
fi

export MPICH_THREAD_LEVEL
AC_DEFINE_UNQUOTED(MPICH_THREAD_LEVEL,$MPICH_THREAD_LEVEL,[Level of thread support selected at compile time])

if test "$MPICH_THREAD_LEVEL" = "MPI_THREAD_MULTIPLE" ; then
   AC_DEFINE([MPICH_IS_THREADED],[1],[MPICH is configured to require thread safety])
fi

# If not in MPI_THREAD_SINGLE, we need -D_REENTRANT to get thread-safe "errno".
# Most POSIX systems provide this by default when using -lpthread, but we only use it in MPI_THREAD_MULTIPLE.
# Some systems (Solaris) still require an explicit define in any case.
if test "$MPICH_THREAD_LEVEL" != "MPI_THREAD_SINGLE"; then
    PAC_APPEND_FLAG([-D_REENTRANT], [CPPFLAGS])
fi

# Check for value thread_cs choice; set the refcount default if necessary
thread_granularity=MPICH_THREAD_GRANULARITY__SINGLE
thread_refcount=MPICH_REFCOUNT__NONE
if test "$enable_threads" = "multiple" ; then
    case $enable_thread_cs in
    global)
    thread_granularity=MPICH_THREAD_GRANULARITY__GLOBAL
    if test "$enable_refcount" = "default" ; then enable_refcount=none ; fi
    ;;
    per-object|per_object)
    thread_granularity=MPICH_THREAD_GRANULARITY__POBJ
    if test "$enable_refcount" = "default" ; then enable_refcount=lock-free ; fi
    ;;
    per-vci|per_vci)
    thread_granularity=MPICH_THREAD_GRANULARITY__VCI
    if test "$enable_refcount" = "default" ; then enable_refcount=lock-free ; fi
    ;;
    lock-free|lock_free|lockfree)
    thread_granularity=MPICH_THREAD_GRANULARITY__LOCKFREE
    if test "$enable_refcount" = "default" ; then enable_refcount=lock-free ; fi
    if test "$enable_predefined_refcount" = "default" ; then enable_predefined_refcount=no ; fi
    AC_MSG_ERROR([--enable-thread-cs=lock-free is not supported yet, please select a different granularity])
    ;;
    *)
    AC_MSG_ERROR([Unrecognized value $enable_thread_cs for --enable-thread-cs])
    ;;
    esac

    case $enable_refcount in
    lock-free|lock_free|lockfree)
    thread_refcount=MPICH_REFCOUNT__LOCKFREE
    ;;
    none)
    thread_refcount=MPICH_REFCOUNT__NONE
    ;;
    *)
    AC_MSG_ERROR([Unrecognized value $enable_refcount for --enable-refcount])
    ;;
    esac
fi
AC_DEFINE_UNQUOTED([MPICH_THREAD_GRANULARITY],$thread_granularity,[Method used to implement atomic updates and access])

if test "$thread_granularity" = "MPICH_THREAD_GRANULARITY__POBJ" -a "$device_name" == "ch4" ; then
    AC_MSG_ERROR([the ch4 device does not support the per-object critical section])
fi

if test "$thread_granularity" = "MPICH_THREAD_GRANULARITY__VCI" -a "$device_name" != "ch4" ; then
    AC_MSG_ERROR([per-vci critical section granularity is only supported for the ch4 device])
fi

if test "$enable_mdta" = "yes" ; then
    if test "$enable_threads" != "multiple" -o "$device_name" != "ch4" -o "$enable_thread_cs" != "global" ; then
        AC_MSG_ERROR([--enable-mdta is not supported for this setting])
    fi
fi

if test "$enable_predefined_refcount" = "no" ; then
    AC_DEFINE([MPICH_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS],[1],[define to disable reference counting predefined objects like MPI_COMM_WORLD])
fi

AC_DEFINE_UNQUOTED([MPICH_THREAD_REFCOUNT],$thread_refcount,[Method used to implement refcount updates])

# enable-g
# strip off multiple options, separated by commas
save_IFS="$IFS"
IFS=","
for option in $enable_g ; do
    case "$option" in
        debug|dbg)
        enable_append_g=yes
	;;
        no|none)
	;;
	handlealloc)
	perform_handlealloc=yes
	;;
	handle)
        perform_handle=yes
	;;
	meminit)
	perform_meminit=yes
	;;
	memarena)
	perform_memarena=yes
	perform_memtracing=yes
	;;
	mem)
	perform_memtracing=yes
	;;
	mutex)
	perform_dbgmutex=yes
	;;
	mutexnesting)
	perform_mutexnesting=yes
	;;
        most|yes)
        perform_memtracing=yes
        enable_append_g=yes
        perform_meminit=yes
        perform_dbgmutex=yes
        perform_mutexnesting=yes
        perform_handlealloc=yes
        perform_handle=yes
        ;;
	all)
        perform_memarena=yes
	perform_memtracing=yes
	enable_append_g=yes
	perform_meminit=yes
	perform_dbgmutex=yes
	perform_mutexnesting=yes
	perform_handlealloc=yes
        perform_handle=yes
	;;
	*)
	IFS=$save_IFS
	AC_MSG_WARN([Unknown value $option for enable-g])
	IFS=","
	;;
    esac
done
IFS="$save_IFS"

if test "$enable_append_g" = "yes" ; then
    PAC_APPEND_FLAG([-g],[CFLAGS])
    PAC_APPEND_FLAG([-g],[CXXFLAGS])
    PAC_APPEND_FLAG([-g],[FFLAGS])
    PAC_APPEND_FLAG([-g],[FCFLAGS])
    PAC_APPEND_FLAG([-g],[WRAPPER_CFLAGS])
    PAC_APPEND_FLAG([-g],[WRAPPER_CXXFLAGS])
    PAC_APPEND_FLAG([-g],[WRAPPER_FFLAGS])
    PAC_APPEND_FLAG([-g],[WRAPPER_FCFLAGS])
fi
if test -n "$perform_meminit" ; then
    AC_DEFINE(MPICH_DEBUG_MEMINIT,1,[Define to enable preinitialization of memory used by structures and unions])
fi
if test "$perform_handlealloc" = yes ; then
    AC_DEFINE(MPICH_DEBUG_HANDLEALLOC,1,[Define to enable checking of handles still allocated at MPI_Finalize])
fi
AS_IF([test "X$perform_handle" = "Xyes"],
      [AC_DEFINE(MPICH_DEBUG_HANDLES,1,[Define to enable handle checking])])

if test -n "$perform_memtracing" ; then
    enable_g_mem=yes
    AC_DEFINE(USE_MEMORY_TRACING,1,[Define to enable memory tracing])
    if test -n "$perform_memarena" ; then
        AC_DEFINE(MPICH_DEBUG_MEMARENA,1,[Define if each function exit should confirm memory arena correctness])
    fi
fi

if test -n "$perform_dbgmutex" ; then
   AC_DEFINE(MPICH_DEBUG_MUTEX,1,[Define to enable mutex debugging])
fi

# POSIX shell will not source file unless it is in the PATH
PATH=$PATH:.
if test "$with_cross" != "no" ; then
    if test -s "$with_cross" ; then
        AC_MSG_RESULT([Reading values from cross-compilation file $with_cross])
        . $with_cross
    fi
fi

# This goes here because we need the top_srcdir
if test "$enable_romio" = "yes" ; then
   if test -d $use_top_srcdir/src/mpi/romio ; then
       subsystems="$subsystems src/mpi/romio"
       AC_DEFINE(HAVE_ROMIO,1,[Define if ROMIO is enabled])

       # make it possible to "#include" mpio.h at build time
       #
       # This ought to be sufficient, but there is also a symlink setup in
       # src/include to accomodate current mpicc limitations.  See
       # src/mpi/Makefile.mk for more info.
       PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpi/romio/include],[CPPFLAGS])

       # Set environment variables that the romio configure expects
       export use_top_srcdir
       top_build_dir=`pwd`
       export top_build_dir
       # if there is no $top_build_dir/lib, romio puts lib in wrong place
       # This test used -e under Linux, but not all test programs understand
       # -e
       if test ! -d lib ; then mkdir lib ; fi
       # tell mpi.h to include mpio.h
       PAC_HAVE_ROMIO
   else
       AC_MSG_WARN([ROMIO src directory is not available])
   fi
fi

AM_CONDITIONAL([BUILD_ROMIO], [test x$enable_romio = xyes])

#
# FIXME: If an external device, don't necessarily complain (e.g.,
# if the device is already built)
if test ! -d $devicedir ; then
    AC_MSG_ERROR([Device $device_name is unknown])
elif test -f $devicedir/subconfigure.m4 ; then
    # this is a new-style subconfigure device, don't add it as a subsystem
    :
else
    # Add the device to the configure list
    devsubsystems="$devsubsystems $devicereldir"
    # Make device_name available to subdirs
fi

if test -n "${with_pmix}" -a "${with_pmix}" != "no" ; then
    # disable built-in PMI and process managers
    with_pmi="no"
    with_pm="no"
    if test "${device_name}" != "ch4" ; then
        AC_MSG_ERROR([$device_name does not support PMIx])
    fi
    PAC_PUSH_FLAG([LIBS])
    PAC_CHECK_HEADER_LIB_FATAL(pmix, pmix.h, pmix, PMIx_Init)
    PAC_APPEND_FLAG([-lpmix],[WRAPPER_LIBS])
    PAC_POP_FLAG([LIBS])
    AC_DEFINE(USE_PMIX_API, 1, [Define if PMIx API must be used])
fi

# with-pm
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

# We allow multiple pm names, separated by : or ,
if test "$with_pm" != "no" ; then
    pm_names="`echo $with_pm | sed -e 's/:/ /g' -e 's/,/ /g'`"
else
    pm_names=""
fi
#
hasError=no
# We need to be careful about PM's that have either conflicting
# requirements (e.g., different PMI implementations) or different
# optional features (e.g., MPID_PM_NAMESERVER).
# In addition, we need to interleave the setup of the PMI and PM
# modules.  The order is as follows:
#
# For each PM, execute the mpichprereq script for that pm (if present).
# This script provides information about the PM, including which PMI
# implementations are supported.
#
# Then, for the selected PMI, the setup script (if any) is run.  This is
# necessary because the setup of the PM may require information discovered
# or provided duing the PMI setup step.
#
# Finally, for each PM, the setup script is executed.
#
# Step 1: invoke the mpichprereq for each PM
for pm_name in $pm_names ; do
    if test -z "$first_pm_name" ; then
       first_pm_name=$pm_name
       export first_pm_name
    fi
    if test ! -d $use_top_srcdir/src/pm/$pm_name ; then
        AC_MSG_WARN([$use_top_srcdir/src/pm/$pm_name does not exist. PM is unknown])
	hasError=yes
    elif test ! -x $use_top_srcdir/src/pm/$pm_name/configure -a \
              ! -f $use_top_srcdir/src/pm/$pm_name/subconfigure.m4 ; then
        if test -s $use_top_srcdir/src/pm/$pm_name/configure ; then
	    AC_MSG_WARN([The configure in $use_top_srcdir/src/pm/$pm_name exists but is not executable])
        else
            AC_MSG_WARN([pm $pm_name has no configure or subconfigure.m4])
        fi
        pm_name=""
	hasError=yes
    else
	nameserver=$MPID_PM_NAMESERVER
        if test -f $use_top_srcdir/src/pm/$pm_name/mpichprereq ; then
	    echo sourcing $use_top_srcdir/src/pm/$pm_name/mpichprereq
	    . $use_top_srcdir/src/pm/$pm_name/mpichprereq
	fi
	# Check for a change; if found, we'll take the default
	if test "$MPID_PM_NAMESERVER" != "$nameserver" ; then
            if test "$first_pm_name" != "$pm_name" ; then
	        # Reject suggestion (use the default, common mode)
		MPID_PM_NAMESERVER=""
	    fi
	fi
    fi
done
if test "$hasError" != no ; then
    AC_MSG_ERROR([Aborting configure because an error was seen in the selection of process managers])
fi
#
# pm_name is the *primary* pm
pm_name=$first_pm_name
AC_SUBST(pm_name)

# Step 2:
# Once we've selected the process manager (or managers), we can
# check that we have a compatible PMI implemenatation.
# with-pmi
if test "$with_pmi" != "no" ; then
    if test "$with_pmi" = "default" -o "$with_pmi" = "yes" ; then
        if test -n "$PM_REQUIRES_PMI" ; then
	    with_pmi=$PM_REQUIRES_PMI
        else
            with_pmi=simple
        fi
    elif test -n "$PM_REQUIRES_PMI" ; then
        # Test for compatibility between pm and pmi choices
        if test "$PM_REQUIRES_PMI" != "$with_pmi" ; then
	    AC_MSG_ERROR([The PM chosen ($with_pm) requires the PMI implementation $PM_REQUIRES_PMI but $with_pmi was selected as the PMI implementation.])
        fi
    fi
    pmi_name=$with_pmi

    if test ! -d $use_top_srcdir/src/pmi/$pmi_name ; then
        AC_MSG_WARN([$use_top_srcdir/src/pmi/$pmi_name does not exist. PMI is unknown])
    elif test ! -x $use_top_srcdir/src/pmi/$pmi_name/configure ; then
        if test ! -f $use_top_srcdir/src/pmi/$pmi_name/subconfigure.m4 ; then
            AC_MSG_WARN([pmi $pmi_name has no configure or subconfigure.m4])
            pmi_name=""
        fi
    else
        # only add to subsystems if a full configure is present
        subsystems="$subsystems src/pmi/$pmi_name"
    fi
fi

# Step 3: complete pm setup.
# Note that checks for errors have already been performed, so this
# loop does not need to perform any extra error checks.
# Note that this uses this_pm_name because pm_name must be the *first*
# of the PM names
for this_pm_name in $pm_names ; do
    # only add the PM to the subsystems if it has a full configure to be
    # executed
    if test -f $use_top_srcdir/src/pm/$this_pm_name/configure ; then
        subsystems="$subsystems src/pm/$this_pm_name"
    fi
    if test -f $use_top_srcdir/src/pm/$this_pm_name/setup_pm ; then
	echo sourcing $use_top_srcdir/src/pm/$this_pm_name/setup_pm
	. $use_top_srcdir/src/pm/$this_pm_name/setup_pm
    fi
done

# Check for whether the compiler defines a symbol that contains the
# function name. The MPICH code uses this for debugging purposes.

# NOTE: C99, C11 defines __func__. If not supported, it will only
# break debugging, not the main code.

PAC_CC_FUNCTION_NAME_SYMBOL

# Check if $MPI_DEFAULT_COPTS is valid with $CC
if test -n "$MPI_DEFAULT_COPTS" ; then
    if test "$enable_check_compiler_flags" = "yes" ; then
        PAC_C_CHECK_COMPILER_OPTION( [$MPI_DEFAULT_COPTS], [
        CFLAGS="$CFLAGS $MPI_DEFAULT_COPTS"
                                   ] )
    else
        CFLAGS="$CFLAGS $MPI_DEFAULT_COPTS"
    fi
fi

# ---------------------------------------------------------------------------
# determine shared library flags for CC
# src/env may not exist yet in a vpath build
$MKDIR_P src/env
cc_shlib_conf=src/env/cc_shlib.conf
PAC_COMPILER_SHLIB_FLAGS([CC],[$cc_shlib_conf])
AC_SUBST_FILE([cc_shlib_conf])

# We need variables from $cc_shlib_conf to set `WRAPPER_C_DYNAMIC_LOADING_FLAGS`, which 
#    is used in `src/packaging/pkgconfig/mpich.pc.in`
# However, libtool's $library_names_spec is more than what's given in $cc_shlib_conf, so
#    we need protect it. 
# FIXME: potentially, there are more variable collisions. Need better plan.
PAC_PUSH_FLAG([library_names_spec])
. $cc_shlib_conf
PAC_POP_FLAG([library_names_spec])

# Attempt to construct dynamic loading info, based on the user
# preference of rpath, runpath or none and on the detected libdir
# flags.
if test "X${with_wrapper_dl_type}" = "Xrunpath" ; then
   eval WRAPPER_C_DYNAMIC_LOADING_FLAGS=\"${hardcode_libdir_flag_spec} ${enable_dtags_flag}\"
elif test "X${with_wrapper_dl_type}" = "Xrpath" ; then
   eval WRAPPER_C_DYNAMIC_LOADING_FLAGS=\"${hardcode_libdir_flag_spec} ${disable_dtags_flag}\"
else
   WRAPPER_C_DYNAMIC_LOADING_FLAGS=""
fi
AC_SUBST(WRAPPER_C_DYNAMIC_LOADING_FLAGS)

# ---------------------------------------------------------------------------
# Support for MPI_T performance variables

# enable-mpit-pvars
# strip off multiple options, separated by commas
save_IFS="$IFS"
IFS=","
for var in $enable_mpit_pvars ; do
    AS_CASE(["$var"],
            [nem],[enable_pvar_nem=yes],
            [recvq],[enable_pvar_recvq=yes],
            [rma],[enable_pvar_rma=yes],
	    [dims],[enable_pvar_dims=yes],
            [all|yes],
            [enable_pvar_nem=yes
             enable_pvar_recvq=yes
             enable_pvar_rma=yes
	     enable_pvar_dims=yes
	     ],
            [no|none],[],
            [IFS=$save_IFS
             AC_MSG_WARN([Unknown value ($option) for enable-mpit-pvars])
             IFS=","])
done
IFS="$save_IFS"

if test -n "$enable_pvar_nem" ; then
    status_nem_pvars=1
else
    status_nem_pvars=0
fi
AC_DEFINE_UNQUOTED(ENABLE_PVAR_NEM,$status_nem_pvars,
          [Define to 1 to enable nemesis-related MPI_T performance variables])

if test -n "$enable_pvar_recvq" ; then
    status_recvq_pvars=1
else
    status_recvq_pvars=0
fi
AC_DEFINE_UNQUOTED(ENABLE_PVAR_RECVQ,$status_recvq_pvars,
          [Define to 1 to enable message receive queue-related MPI_T performance variables])

if test -n "$enable_pvar_rma" ; then
    status_rma_pvars=1
else
    status_rma_pvars=0
fi
AC_DEFINE_UNQUOTED(ENABLE_PVAR_RMA,$status_rma_pvars,
          [Define to 1 to enable rma-related MPI_T performance variables])

if test -n "$enable_pvar_dims" ; then
    status_dims_pvars=1
else
    status_dims_pvars=0
fi
AC_DEFINE_UNQUOTED(ENABLE_PVAR_DIMS,$status_dims_pvars,
          [Define to 1 to enable getdims-related MPI_T performance variables])
# ---------------------------------------------------------------------------
# Support for the language bindings: Fortran 77, Fortran 90, and C++
#
# First, we handle the case of no explicit enable/disable option.  In that
# case, we look for a usable compiler.  We cannot use the ac macros for this
# because they abort the configure step if they fail to find a compiler
# (earlier versions of autoconf did not have this behavior!).
#
# Second, we perform the langugage-specific tests, if necessary.  This may
# be relatively simple (C++) or complex (Fortran 77, including formation of
# the encoded MPI handles).
#
# Note that the bindings support needs to know some of the properties of
# the C compiler, so those tests (particularly for weak symbols)
# must come first.
# ----------------------------------------------------------------------------

#
# First, determine whether we are/can support the language bindings
#
# Since F90/F90FLAGS are replaced by FC/FCFLAGS, rather than silently
# substituting them, i.e. FC=$F90 and FCFLAGS=$F90FLAGS, we choose to emit
# an error message and abort to avoid any ambiguous/hidden bug in choosing
# Fortran90 compilers.
if test -n "$F90" -o -n "$F90FLAGS" ; then
    AC_MSG_ERROR([F90 and F90FLAGS are replaced by FC and FCFLAGS respectively in this configure, please unset F90/F90FLAGS and set FC/FCFLAGS instead and rerun configure again.])
fi
# ----------------------------------------------------------------------------
# Handle default choices for the Fortran compilers
# Note that these have already been set above

if test "$enable_f77" = "yes"; then
    if test "$F77" = "" -o "$F77" = "no"; then
        # No Fortran 77 compiler found; abort
        AC_MSG_ERROR([No Fortran 77 compiler found. If you don't need to
        build any Fortran programs, you can disable Fortran support using
        --disable-fortran. If you do want to build Fortran
        programs, you need to install a Fortran compiler such as gfortran
        or ifort before you can proceed.])
    fi
fi

if test "$enable_f77" = yes ; then
    # Check if $MPI_DEFAULT_FOPTS is valid with $F77
    if test -n "$MPI_DEFAULT_FOPTS" ; then
        if test "$enable_check_compiler_flags" = "yes" ; then
            PAC_F77_CHECK_COMPILER_OPTION( [$MPI_DEFAULT_FOPTS], [
            FFLAGS="$FFLAGS $MPI_DEFAULT_FOPTS"
                                         ] )
        else
            FFLAGS="$FFLAGS $MPI_DEFAULT_FOPTS"
        fi
    fi
fi

#
# We need to know the name mangling for Fortran before testing for FC
# compatibility (we need this because of the way we decide to generate
# the Fortran 77 bindings)
if test "$enable_f77" = yes ; then
    FLIBS_save="$FLIBS"
    FLIBS=""
    AC_F77_LIBRARY_LDFLAGS
    # The autoconf macro for finding FLIBS sometimes makes mistakes
    # (particularly with the Fujitsu frt compiler).  This next step
    # first sees if the FLIBS is valid with the Fortran compiler
    PAC_PROG_F77_FLIBS_VALID
    # Now see if FLIBS works with the C compiler
    PAC_PROG_F77_CHECK_FLIBS

    # Check F77+FLAGS is compatible with CC+CFLAGS before using F77+CC.
    PAC_PROG_F77_OBJ_LINKS_WITH_C

    PAC_PROG_F77_LINKER_WITH_C
    # For name mangle, we need the library flags
    PAC_PROG_F77_NAME_MANGLE
    # Check whether additional libraries are needed when linking with C
    PAC_PROG_F77_AND_C_STDIO_LIBS
    AC_SUBST(F77_OTHER_LIBS)

    # Warn about mixed name mangling, since many of the tests will fail
    if test "$pac_cv_prog_f77_name_mangle" = "mixed" ; then
        AC_MSG_WARN([The compiler $F77 uses mixed case names.  Fortran is monocase
and many Fortran programs may use either upper or lower case names for MPI
calls.  Consider specifying a particular parameter to your Fortran compiler
to select either upper or lower case names.  For the Absoft compiler,
-f selects lower case and -N109 selects upper case (if you use -f, also use
-B108 to enable the iargc and getarg routines, which are needed for some
tests and by many user programs).  Specify new command
line options by setting the environment variable FFLAGS to include
the options (e.g., setenv FFLAGS "-f -B108").  In addition, make sure that your
Fortran 90 compiler uses a compatible naming choice.  For the
Absoft Fortran 90, -YALL_NAMES=LCS selects lower case names and -B108
adds underscores to names, as required for iargc and getarg.  Pass this
information to configure with the FCFLAGS environment variable.])
        # If Fortran implicitly enabled, disable it now.  Otherwise,
	# abort the configure since warning messages are often lost in
	# the output.
	AC_MSG_ERROR([Aborting configure because of mixed case names in Fortran.  Either select --disable-fortran or set FCFLAGS to force the compiler to select monocase names])
    fi

    # The MPI standard requires that MPI_Init in any language initialize
    # MPI in all languages.  This can be a problem when objects produced
    # by the Fortran compiler require symbols from the Fortran runtime
    # (making linking C-only programs unnecessarily difficult).  What we test
    # here is whether the much more restricted needs of the Fortran
    # initialize can be met with no special use of the Fortran runtime
   PAC_F77_INIT_WORKS_WITH_C
   if test "$pac_f_init_works_with_c" = "yes" ; then
       AC_DEFINE(HAVE_MPI_F_INIT_WORKS_WITH_C,1,[Define if the Fortran init code for MPI works from C programs without special libraries])
   fi

   #
   # Some Fortran compilers now pass CHARACTER length as a size_t instead
   # of as an int.  This is hard to test for, since the data is passed by
   # value and for characters less than about 2GB long, the correct
   # value will be used.  In this case, we must use an approach similar to
   # the one used by libtool for shared library options - look at the
   # compiler name or vendor.
   # Known compilers that use size_t instead of int:
   #    Intel Fortran
   #    gfortran
   # Add others as they become known
   AC_ARG_ENABLE(f77characterlen,
       AC_HELP_STRING([--enable-f77characterlen],
           [Select between int and size_t for the length of a Fortran CHARACTER, depending on the F77 compiler.  If --enable-f77characterlen=size_t is given, force the use of size_t.  This is used for passing Fortran CHARACTER data between C and Fortran, and is provided for experts.  Note that the documentation provided by compiler vendors on the calling convention may not be accurate.]),,enable_f77characterlen=no)

   # Set the default
   f77_uses_int_for_str=default

   case "$enable_f77characterlen" in
      yes|no)
      ;;
      size_t)
         f77_uses_int_for_str=no
	 enable_f77characterlen=yes
      ;;
      int)
         f77_uses_int_for_str=yes
	 enable_f77characterlen=yes
      ;;
      *)
      AC_MSG_ERROR([Invalid value provided for --enable-f77characterlen])
      ;;
   esac

   # If we might change the size (enable) and we haven't set the choice,
   # attempt to determine it from the compiler name.  Risky, but we haven't
   # found a reliable way to do this with test codes.
   if test "$enable_f77characterlen" = "yes" -a \
           "$f77_uses_int_for_str" = "default" ; then
       f77_uses_int_for_str=yes
       f77Basename=`basename $F77`
       case $f77Basename in
       ifort*)
       f77_uses_int_for_str=no
       ;;
       gfortran*)
       f77_uses_int_for_str=no
       ;;
       esac
   fi
   # This test is disabled for now.  Despite information in documentation
   # on gfortran, it appears to pass lengths as int, at least in some
   # builds (it used movl when tested in 2/2013).  Tests that failed
   # included infotestf.f, in a call to mpi_info_get.
   # Leave this as a place holder until a proper test can be determined.
   if test  "$enable_f77characterlen" = "yes" -a \
            "$f77_uses_int_for_str" = "no" ; then
       AC_DEFINE(USE_FORT_STR_LEN_SIZET,1,[Define if the length of a CHARACTER*(*) string in Fortran should be passed as size_t instead of int] )
   fi

fi

# FC requires F77 as well.  If the user disabled f77, do not run the
# next test; instead, drop into the warning message
# Set a default value for fc works with f77.  This value is
# set to no *only* if fc was selected but was not compatible with f77
fc_with_f77=yes
if test "$enable_fc" = "yes" -a "$enable_f77" = yes ; then
    enable_fc=no
    if test "$FC" != "no" ; then
	# If we allow multiple weak symbols, we should test a name
	# that does not contain an underscore.  The Fortran binding uses
	# this rule for enabling multiple weak symbols:
	# if defined(USE_WEAK_SYMBOLS) && !defined(USE_ONLY_MPI_NAMES) &&
	#    defined(HAVE_MULTIPLE_PRAGMA_WEAK) &&
	#    defined(F77_NAME_LOWER_2USCORE)
	#
	testRoutine="t1_2"
	if test "$pac_cv_prog_c_multiple_weak_symbols" = "yes" -a \
               "$enable_weak_symbols" = "yes" -a \
	       "$pac_cv_prog_f77_name_mangle" = "lower doubleunderscore" ; then
	    testRoutine="t12"
        fi
        PAC_FC_AND_F77_COMPATIBLE(fc_with_f77=yes,fc_with_f77=no,$testRoutine)
        if test "$fc_with_f77" != yes ; then
	    enable_fc=no
	    AC_MSG_ERROR([The selected Fortran 90 compiler $FC does not work with the selected Fortran 77 compiler $F77.  Use the environment variables FC and F77 respectively to select compatible Fortran compilers.  The check here tests to see if a main program compiled with the Fortran 90 compiler can link with a subroutine compiled with the Fortran 77 compiler.])
        elif test "$fc_with_f77" = "yes" ; then
            # If we got here, there is a Fortran 90 compiler that we can use
            enable_fc=yes
        fi
    elif test "$pac_cv_prog_fc_works" = no; then
        AC_MSG_WARN([Use --disable-fc to keep configure from searching for a Fortran 90 compiler])
    fi

    if test "$enable_fc" = "yes"; then
        if test "$FC" = "no" -o "$FC" = ""; then
            # No Fortran 90 compiler found; abort
            AC_MSG_ERROR([No Fortran 90 compiler found. If you don't need
            to build any Fortran 90 programs, you can disable Fortran 90
            support using --disable-fc. If you do want to build Fortran 90
            programs, you need to install a Fortran 90 compiler such as
            gfortran or ifort before you can proceed.])
        fi
    fi
fi

if test "$enable_fc" = "yes" -a "$enable_f77" != "yes" ; then
   # Fortran 90 support requires compatible Fortran 77 support
   AC_MSG_ERROR([
Fortran 90 support requires compatible Fortran 77 support.
To force the use of the Fortran 90 compiler for Fortran 77,
do not use configure option --disable-fortran, and set the environment
variable F77 to the name of the Fortran 90 compiler, or \$FC.
If you do not want any Fortran support, use configure options
--disable-fortran.])
   # We should probably do the compatibility test as well
   enable_f77=yes
fi

# ----------------------------------------------------------------------------
# Now test for Fortran compiler characteristics
# ----------------------------------------------------------------------------
if test "$enable_f77" = "yes" ; then
    # determine shared library flags for F77
    f77_shlib_conf=src/env/f77_shlib.conf
    PAC_COMPILER_SHLIB_FLAGS([F77],[$f77_shlib_conf])
    AC_SUBST_FILE([f77_shlib_conf])

    AC_LANG_FORTRAN77
    PAC_PROG_F77_EXCLAIM_COMMENTS(has_exclaim="yes",has_exclaim="no")
    PAC_PROG_F77_HAS_INCDIR(src)
    PAC_PROG_F77_LIBRARY_DIR_FLAG

    AC_SUBST(MPIFPMPI)
    if test "$MPI_WITH_PMPI" = "no" ; then
        # If the PMPI routines are not in the same library with the MPI
        # routines, we may need to remove the pmpi declarations
        PAC_PROG_F77_ALLOWS_UNUSED_EXTERNALS([MPIFPMPI=",PMPI_WTIME,PMPI_WTICK"],[
        MPIFPMPI="";
        AC_MSG_WARN([Removed PMPI_WTIME and PMPI_WTICK from mpif.h])])
    else
        MPIFPMPI=",PMPI_WTIME,PMPI_WTICK"
    fi

    # Once we have name mangle, we can try to limit the number of needed libs
    dnl F77_IN_C_LIBS is not needed currently because mpirinitf_() in setbotf.f
    dnl is called in initthread.c only when FLIBS is not needed to initialize
    dnl Fortran constants from a C main, See PAC_F77_INIT_WORKS_WITH_C.
    dnl PAC_PROG_F77_IN_C_LIBS
    dnl AC_SUBST(F77_IN_C_LIBS)

    # Most systems allow the Fortran compiler to process .F and .F90 files
    # using the C preprocessor.  However, some systems either do not
    # allow this or have serious bugs (OSF Fortran compilers have a bug
    # that generates an error message from cpp).  The following test
    # checks to see if .F works, and if not, whether "cpp -P -C" can be used
    # This is needed for Mac OSX 10.5
    PAC_F77_WORKS_WITH_CPP([F77CPP])
    AC_SUBST(F77CPP)

    # Check that the Fortran compiler will allow us to pass arguments
    # of different types (e.g., for MPI_Send)
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

    bindings="$bindings f77"
    AC_DEFINE(HAVE_FORTRAN_BINDING,1,[Define if Fortran is supported])
    # Also define the name FORTRAN_BINDING for use in #if @FORTRAN_BINDING@..
    FORTRAN_BINDING=1

fi

dnl By modifying mpif.h to use ! for comments, it can work with many f90
dnl compilers without creating a separate version.

AC_CONFIG_COMMANDS([filter], [perl $srcdir/maint/config_filter.pl], [
export has_exclaim=$has_exclaim
export has_fort_real8=$has_fort_real8
])

if test "$enable_fc" = "yes" ; then
    if test "$enable_f77" != "yes" ; then
        AC_MSG_WARN([Fortran 90 requires Fortran 77])
        enable_fc=no
    else
        bindingsubsystems="$bindingsubsystems src/binding/fortran/use_mpi"
        bindings="$bindings f90"
    fi
fi

f08_works=no
if test "$enable_fc" = "yes" ; then
    PAC_FC_2008_SUPPORT([f08_works=yes],[f08_works=no])
fi
AM_CONDITIONAL([BUILD_F08_BINDING], [test "$f08_works" = "yes"])

if test "$f08_works" = "yes" ; then
    status_f08_works=1
    bindings="$bindings f08"
else
    status_f08_works=0
fi
AC_DEFINE_UNQUOTED(HAVE_F08_BINDING, $status_f08_works, [Define to 1 to enable Fortran 2008 binding])

# Set defaults for these values so that the Makefile in src/bindings/f90
# is valid even if fc is not enabled (this is necessary for the
# distclean target)
MPIMODNAME=mpi
MPICONSTMODNAME=mpi_constants
MPISIZEOFMODNAME=mpi_sizeofs
MPIBASEMODNAME=mpi_base

# F08 binding stuff
MPI_F08_LINK_CONSTANTS_NAME=mpi_f08_link_constants
PMPI_F08_NAME=pmpi_f08
MPI_F08_CALLBACKS_NAME=mpi_f08_callbacks
MPI_C_INTERFACE_NAME=mpi_c_interface
MPI_C_INTERFACE_GLUE_NAME=mpi_c_interface_glue
MPI_F08_TYPES_NAME=mpi_f08_types
MPI_C_INTERFACE_NOBUF_NAME=mpi_c_interface_nobuf
MPI_F08_COMPILE_CONSTANTS_NAME=mpi_f08_compile_constants
MPI_F08_NAME=mpi_f08
MPI_C_INTERFACE_TYPES_NAME=mpi_c_interface_types
MPI_C_INTERFACE_CDESC_NAME=mpi_c_interface_cdesc

if test "$enable_fc" = "yes" ; then
    # determine shared library flags for FC
    fc_shlib_conf=src/env/fc_shlib.conf
    PAC_COMPILER_SHLIB_FLAGS([FC],[$fc_shlib_conf])
    AC_SUBST_FILE([fc_shlib_conf])

    # Determine characteristics of the Fortran 90 compiler
    # Find a Fortran 90 compiler.  Sets FC
    # Work around bug in autoconf that adds -g to FCFLAGS
    saveFCFLAGS="$FCFLAGS"
    dnl FIXME XXX DJG this needs to be reconciled with our separate use of
    dnl AC_PROG_FC earlier
    dnl PAC_PROG_FC
    PAC_PROG_FC_WORKS
    FCFLAGS=$saveFCFLAGS
    if test "$pac_cv_prog_fc_works" = no ; then
        # Reject this compiler
        if test "$FC" != "no" ; then
            fc_rejected=yes
            oldFC="$FC"
            FC="no"
        fi
    fi

    # Determine the extension for Fortran 90 files (it isn't always .f90)
    FCEXT=$ac_fc_srcext
    AC_SUBST(FCEXT)

    if test "$FC" = "no" ; then
        if test "$fc_rejected" = "yes" ; then
            AC_MSG_ERROR([Could not find a usable Fortran 90 compiler.  The compiler $oldFC may be incompatible with the Fortran 77 compiler $F77; check the output of configure and consult the installation manuals])
        else
            AC_MSG_ERROR([Could not find a usable Fortran 90 compiler.])
        fi
    fi

    # Find the extension that this compiler uses for modules.
    # Sets FCMODEXT (and adds it to the list substed)
    # Sets FCMODINCFLAG (and adds it to the list substed)
    PAC_FC_MODULE
    AC_SUBST(FCMODINCSPEC)
    if test -z "$FCMODOUTFLAG" ; then
        AC_MSG_ERROR([FCMODOUTFLAG could not be determined but is critical for the current Fortran build system])
    fi

    if test "$pac_cv_fc_module_case" = "upper" ; then
        MPIMODNAME=MPI
        MPICONSTMODNAME=MPI_CONSTANTS
	MPISIZEOFMODNAME=MPI_SIZEOFS
	MPIBASEMODNAME=MPI_BASE
        MPI_F08_NAME=MPI_F08
        MPI_F08_LINK_CONSTANTS_NAME=MPI_F08_LINK_CONSTANTS
        MPI_F08_CALLBACKS_NAME=MPI_F08_CALLBACKS
        MPI_F08_TYPES_NAME=MPI_F08_TYPES
        MPI_F08_COMPILE_CONSTANTS_NAME=MPI_F08_COMPILE_CONSTANTS
        PMPI_F08_NAME=PMPI_F08
        MPI_C_INTERFACE_NAME=MPI_C_INTERFACE
        MPI_C_INTERFACE_NOBUF_NAME=MPI_C_INTERFACE_NOBUF
        MPI_C_INTERFACE_GLUE_NAME=MPI_C_INTERFACE_GLUE
        MPI_C_INTERFACE_TYPES_NAME=MPI_C_INTERFACE_TYPES
        MPI_C_INTERFACE_CDESC_NAME=MPI_C_INTERFACE_CDESC
    else
        MPIMODNAME=mpi
        MPICONSTMODNAME=mpi_constants
	MPISIZEOFMODNAME=mpi_sizeofs
	MPIBASEMODNAME=mpi_base
        MPI_F08_NAME=mpi_f08
        MPI_F08_LINK_CONSTANTS_NAME=mpi_f08_link_constants
        MPI_F08_CALLBACKS_NAME=mpi_f08_callbacks
        MPI_F08_TYPES_NAME=mpi_f08_types
        MPI_F08_COMPILE_CONSTANTS_NAME=mpi_f08_compile_constants
        PMPI_F08_NAME=pmpi_f08
        MPI_C_INTERFACE_NAME=mpi_c_interface
        MPI_C_INTERFACE_NOBUF_NAME=mpi_c_interface_nobuf
        MPI_C_INTERFACE_GLUE_NAME=mpi_c_interface_glue
        MPI_C_INTERFACE_TYPES_NAME=mpi_c_interface_types
        MPI_C_INTERFACE_CDESC_NAME=mpi_c_interface_cdesc
    fi
    AC_SUBST(MPIMODNAME)
    AC_SUBST(MPICONSTMODNAME)
    AC_SUBST(MPISIZEOFMODNAME)
    AC_SUBST(MPIBASEMODNAME)

    AC_SUBST(MPI_F08_NAME)
    AC_SUBST(MPI_F08_LINK_CONSTANTS_NAME)
    AC_SUBST(MPI_F08_CALLBACKS_NAME)
    AC_SUBST(MPI_F08_TYPES_NAME)
    AC_SUBST(MPI_F08_COMPILE_CONSTANTS_NAME)
    AC_SUBST(PMPI_F08_NAME)
    AC_SUBST(MPI_C_INTERFACE_NAME)
    AC_SUBST(MPI_C_INTERFACE_NOBUF_NAME)
    AC_SUBST(MPI_C_INTERFACE_GLUE_NAME)
    AC_SUBST(MPI_C_INTERFACE_TYPES_NAME)
    AC_SUBST(MPI_C_INTERFACE_CDESC_NAME)

    # Assume that all Fortran 90 compilers accept -I for include directories
    FCINC=-I
    AC_SUBST(FCINC)
    FCINCFLAG=-I
    AC_SUBST(FCINCFLAG)

    # Check if $MPI_DEFAULT_FCOPTS is valid with $F90
    if test -n "$MPI_DEFAULT_FCOPTS" ; then
        if test "$enable_check_compiler_flags" = "yes" ; then
            PAC_FC_CHECK_COMPILER_OPTION( [$MPI_DEFAULT_FCOPTS], [
            FCFLAGS="$FCFLAGS $MPI_DEFAULT_FCOPTS"
                                     ] )
        else
            FCFLAGS="$FCFLAGS $MPI_DEFAULT_FCOPTS"
        fi
    fi

    # Most systems allow the Fortran compiler to process .F and .F90 files
    # using the C preprocessor.  However, some systems either do not
    # allow this or have serious bugs (OSF Fortran compilers have a bug
    # that generates an error message from cpp).  The following test
    # checks to see if .F works, and if not, whether "cpp -P -C" can be used
    PAC_FC_WORKS_WITH_CPP([FCCPP])
    AC_SUBST(FCCPP)

    # Check whether additional libraries are needed when linking with C
    PAC_PROG_FC_AND_C_STDIO_LIBS
    AC_SUBST(FC_OTHER_LIBS)
    # ------------------------------------------------
fi

if test "X$modincdir" = "X" ; then
  modincdir=$includedir
fi
export modincdir
AC_SUBST(modincdir)

AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether TRUE has been defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[return TRUE;])],
                  [has_true_upper_case=yes],
	          [has_true_upper_case=no])
AC_MSG_RESULT($has_true_upper_case)
if test "$has_true_upper_case" = "no" ; then
   AC_DEFINE(TRUE,1,[Define TRUE])
   AC_DEFINE(FALSE,0,[Define FALSE])
fi
AC_LANG_POP([C])

# ----------------------------------------------------------------------------
# We previously allowed "default" as a valid value for $enable_cxx.  Now we
# assume its available by default and error out if it doesn't work (just like
# F77 & FC).  The user must pass "--disable-cxx" in order to successfully
# complete configure in this case.
if test "$enable_cxx" = "default" ; then
    AC_MSG_ERROR([aborting because "--enable-cxx=default" is no longer a supported value])
fi

if test "$enable_cxx" = "yes" ; then
    # Another bug in autoconf.  The checks for the C++ compiler do not
    # ensure that you can link a program that is built with the C++
    # compiler.  We've seen this error with gcc and icc, particularly
    # when those compilers accept C++ language elements but are unable
    # to link programs that are really C++.  For that reason,
    # we've added a test to see if the C++ compiler can produce
    # an executable.
    AC_CACHE_CHECK([whether the C++ compiler $CXX can build an executable],
    pac_cv_cxx_builds_exe,[
 AC_LANG_PUSH([C++])
 AC_TRY_LINK([
class mytest {
  int a;
public:
  mytest(void) : a(1) {}
  ~mytest(void) {}
};],[mytest a;],
 pac_cv_cxx_builds_exe=yes, pac_cv_cxx_builds_exe=no)
 AC_LANG_POP([C++])
])
    if test "$pac_cv_cxx_builds_exe" != yes ; then
        AC_MSG_ERROR([Aborting because C++ compiler does not work.  If you do not need a C++ compiler, configure with --disable-cxx])
    fi
    # Recent failures have come when a standard header is loaded
    # The Intel icpc 10.x compiler fails with <string> if gcc 4.2 is installed.
    AC_CACHE_CHECK([whether C++ compiler works with string],pac_cv_cxx_compiles_string,[
    AC_LANG_PUSH([C++])
    AC_TRY_COMPILE([#include <string>],[return 0;],pac_cv_cxx_compiles_string=yes,pac_cv_cxx_compiles_string=no)
    AC_LANG_POP([C++])
])
    if test "$pac_cv_cxx_compiles_string" != yes ; then
        AC_MSG_WARN([The C++ compiler $CXX cannot compile a program containing the <string> header - this may indicate a problem with the C++ installation.  Consider configuing with --disable-cxx])
    fi

    AC_LANG_CPLUSPLUS
    AX_CXX_EXCEPTIONS
    AX_CXX_BOOL
    AX_CXX_NAMESPACES
    if test "$ac_cv_cxx_namespaces" != "yes" ; then
        AC_MSG_ERROR([Namespaces are required for the MPI C++ interface])
    fi
    HAVE_CXX_EXCEPTIONS=0
    AC_SUBST(HAVE_CXX_EXCEPTIONS)
    if test "$ac_cv_cxx_exceptions" = "yes" ; then
        HAVE_CXX_EXCEPTIONS=1
    fi
    # iostream and math are needed for examples/cxx/cxxpi.cxx
    AC_CACHE_CHECK([whether <iostream> available],pac_cv_cxx_has_iostream,[
    AC_TRY_COMPILE([
#include <iostream>
],[using namespace std;],pac_cv_cxx_has_iostream=yes,pac_cv_cxx_has_iostream=no)])
    AX_CXX_NAMESPACE_STD

    AC_CACHE_CHECK([whether <math> available],pac_cv_cxx_has_math,[
    AC_TRY_COMPILE([
#include <math>
],[using namespace std;],pac_cv_cxx_has_math=yes,pac_cv_cxx_has_math=no)])

    # GNU changed the calling conventions between minor releases (!!!)
    # This is too hard to detect, but we should be able to detect
    # the version mismatch.  By default, we set the GNU version to 0.
    # In a cross-compiling environment, these can be set with environment
    # variables, either directly or through the standard "CROSS" variable.
    if test -z "$GNUCXX_VERSION" ; then
        if test -n "$CROSS_GNUCXX_VERSION" ; then
	     GNUCXX_VERSION=$CROSS_GNUCXX_VERSION
        else
             GNUCXX_VERSION=0
        fi
    fi
    if test -z "$GNUCXX_MINORVERSION" ; then
	if test -n "$CROSS_GNUCXX_MINORVERSION" ; then
	     GNUCXX_MINORVERSION=$CROSS_GNUCXX_MINORVERSION
       	else
             GNUCXX_MINORVERSION=0
        fi
    fi
    if test "$cross_compiling" = "no" -a "$ac_compiler_gnu" = "yes" -a \
       "$GNUCXX_VERSION" = 0 -a "$GNUCXX_MINORVERSION" = 0 ; then
         ac_vals=""
	 AC_MSG_CHECKING([for GNU g++ version])
         AC_TRY_RUN([#include <stdio.h>
int main() {
    int v = -1, m = -1;
    FILE *fp = fopen("conftest.out","w");
#ifdef __GNUC_MINOR__
    m = __GNUC_MINOR__;
#endif
#ifdef __GNUC__
    v = __GNUC__;
#endif
    fprintf( fp, "v=%d, m=%d\n", v, m );
    fclose( fp );
    return 0;
}],ac_vals=`cat conftest.out`)
         if test -n "$ac_vals" ; then
             v=`echo "$ac_vals" | sed -e 's/v=\(.*\),.*/\1/'`
             m=`echo "$ac_vals" | sed -e 's/.*m=\(.*\)/\1/'`
             AC_MSG_RESULT([$v . $m])
             GNUCXX_VERSION=$v
             GNUCXX_MINORVERSION=$m
         else
             AC_MSG_RESULT([unknown])
         fi
    fi
    AC_SUBST(GNUCXX_VERSION)
    AC_SUBST(GNUCXX_MINORVERSION)

    bindings="$bindings cxx"
    AC_DEFINE(HAVE_CXX_BINDING,1,[Define if C++ is supported])
    INCLUDE_MPICXX_H='#include "mpicxx.h"'
    AC_SUBST(INCLUDE_MPICXX_H)

    # In order to support the Fortran datatypes within C++,
    #
    # FORTRAN_BINDING always has a CPP-time value of either 0 or 1,
    # so that it may be used in #if statements without adding to
    # the CPP name space
    AC_SUBST(FORTRAN_BINDING)

    # determine shared library flags for CXX
    cxx_shlib_conf=src/env/cxx_shlib.conf
    PAC_COMPILER_SHLIB_FLAGS([CXX],[$cxx_shlib_conf])
    AC_SUBST_FILE([cxx_shlib_conf])
fi

if test "$enable_cxx" = yes; then
    # Check if $MPI_DEFAULT_CXXOPTS is valid with $CXX
    if test -n "$MPI_DEFAULT_CXXOPTS" ; then
        if test "$enable_check_compiler_flags" = "yes" ; then
            PAC_CXX_CHECK_COMPILER_OPTION( [$MPI_DEFAULT_CXXOPTS], [
            CXXFLAGS="$CXXFLAGS $MPI_DEFAULT_CXXOPTS"
                                     ] )
        else
            CXXFLAGS="$CXXFLAGS $MPI_DEFAULT_CXXOPTS"
        fi
    fi
fi

AC_SUBST(bindings)

# ----------------------------------------------------------------------------
# End of the bindings support
# ----------------------------------------------------------------------------

AC_LANG_C
#
# ----------------------------------------------------------------------------
# Done with the basic argument processing and decisions about which
# subsystems to build
# ----------------------------------------------------------------------------

# Look for perl.  Perl is used *only* in the tests of the commands such as
# mpiexec, mpicc, etc, in test/commands, and in some of the utility
# programs for processing log files .  If perl is not found,
# MPICH may still be built and used.
# We need the full path to perl since we'll use it as the interpreter for
# a shell script.
AC_PATH_PROG(PERL,perl)

# Check for the killall program; this can be used in some of the tests
# in test/commands
AC_CHECK_PROGS(KILLALL,killall,true)

# Does xargs need the -r option to handle the case where the input
# is empty (gnu utils do, Mac OSX does not accept -r)
xargs_out=`echo "" | xargs ls | wc -l | sed -e 's/ //g'`
if test "$xargs_out" != "0" ; then
    XARGS_NODATA_OPT=-r
fi
AC_SUBST(XARGS_NODATA_OPT)

AC_PROG_INSTALL
PAC_PROG_CHECK_INSTALL_WORKS
#
# We also need mkdir -p.
PAC_PROG_MKDIR_P
PAC_PROG_MAKE

#
# Check for bash to allow more robust shell scripts
AC_PATH_PROG(BASH_SHELL,bash)
#
# Confirm that bash has working arrays.  We can use this to
# build more robust versions of the scripts (particularly the
# compliation scripts) by taking advantage of the array features in
# bash.
bashWorks=no
if test -x "$BASH_SHELL" ; then
changequote(%%,::)dnl
    cat >>conftest <<EOF
#! $BASH_SHELL
A[0]="b"
A[1]="c"
rc=1
if test \${A[1]} != "c" ; then rc=2 ; else rc=0 ; fi
exit \$rc
EOF
changequote([,])dnl
    AC_MSG_CHECKING([whether $BASH_SHELL supports arrays])
    chmod +x conftest
    if ./conftest 2>&1 >/dev/null ; then
        bashWorks=yes
    else
        bashWorks=no
    fi
    rm -f conftest*
    AC_MSG_RESULT($bashWorks)
fi
dnl BUILD_BASH_SCRIPTS used to be an AC_SUBST
AM_CONDITIONAL([BUILD_BASH_SCRIPTS], [test "x$bashWorks" = xyes])

# ----------------------------------------------------------------------------
# At this point, we've finally settled on the value of PMPILIBNAME.  We
# can now set NEEDSPLIB.
if test "$NEEDSPLIB" = yes -a "$PMPILIBNAME" = "$MPILIBNAME" ; then
    NEEDSPLIB=no
fi

# We are cheating a bit here and reaching inside of the libtool macros to
# extract the correct shared library extension.  It would be nice if this
# were publicly available in at least the same way that $libext is.
eval SHLIB_EXT='"'$shrext_cmds'"'
export SHLIB_EXT
AC_SUBST(SHLIB_EXT)

# ----------------------------------------------------------------------------
#
# Add the steps for debugger support
BUILD_TVDLL=no
if test "$enable_debuginfo" = "yes" ; then
   # We can build the Totalview interface DLL only if we know how to build
   # shared libraries.

   # FIXME is this really the right test?
   # No.  Before MPICH 1.5, there was the capability to build the debugger
   # libraries without forcing the build of shared libraries for everything.
   # There may be some way to restore this capability, but until then, we
   # at least cause the configure to cleanly fail with a clear error message
   if test "X$enable_shared" = "Xyes" ; then
       BUILD_TVDLL=yes
   else
       AC_MSG_ERROR([Building with --enable-debuginfo now requires building with shared library support.  Add --enable-shared and reconfigure])
   fi

   # One more nasty problem.  Totalview relies on debugger symbols
   # being present in the executable.  Some experimental versions of
   # gcc (3.2 20020329 for ia64) do *not* include the object symbols
   # when debugging.  For HPUX, the necessary linking options are
   #    +noobjdebug
   # for C, Fortran, and C++.  We don't have a good test for this yet,
   # so we add a warning
   if test "$ac_cv_prog_gcc" = "yes" ; then
       AC_MSG_WARN([Some versions of gcc do not include debugging information
within the executable.  Totalview requires this information to detect
an MPICH code.  If you have trouble, try linking with the additional
option
    +noobjdebug
on all link lines (consider adding it to LDFLAGS)])
   fi

    # The debugger library name cannot be set until we know the extension
    # of shared libraries - the name is so on most Unix system, dylib on OS X.
    AC_DEFINE(HAVE_DEBUGGER_SUPPORT,1,[Define if debugger support is included])
    # The debugger support requires a shared library.  This is handled
    # below, after we check for compiler support for shared libraries
    # Note: if libdir contains exec_prefix, handle the fact that the
    # default exec_prefix is NONE, which (much later in configure)
    # gets turned into the value of prefix
    ##ENVVAR: MPICH_DEBUGLIBNAME - Set this environment variable to
    ## override the default name of the debugger support library.
    ## The default name is libtvmpich.$SHLIB_EXT (e.g., libtvmpich.so for
    ## most Unix versions, libtvmpich.dylib for Mac OSX).
    ##ENVVAR END:

    # We need when building CH4. The debugger interface need to build with CH4
    # AM message queue which is not exposed through abstract device interface.
    # This is very hacky (unfortunately) but sufficient for now.
    if test "$device_name" = "ch4" ; then
        AC_DEFINE(HAVE_CH4_DEBUGGER_SUPPORT,1,[Define if debugger support is included for CH4])
    fi

    if test -z "$MPICH_DEBUGLIBNAME" ; then
        DEBUGLIBNAME=libtvmpich$SHLIB_EXT
    else
        # FIXME DJG I don't think this can be supported arbitrarily by the new
        # build system (I'm not sure it was supported correctly by the old
        # system either)
        AC_MSG_WARN([overriding MPICH_DEBUGLIBNAME is currently untested and probably does not work])
        DEBUGLIBNAME=$MPICH_DEBUGLIBNAME
    fi
    if test "x$exec_prefix" = xNONE ; then
        saveExecPrefix=$exec_prefix
	exec_prefix=$prefix
        eval dlldir=$libdir/$DEBUGLIBNAME
	exec_prefix=$saveExecPrefix
    else
        eval dlldir=$libdir/$DEBUGLIBNAME
    fi
    dlldir='"'$dlldir'"'
    AC_DEFINE_UNQUOTED(MPICH_INFODLL_LOC,$dlldir,[Define as the name of the debugger support library])
fi

# used by automakefiles to conditionally build the DLL
AM_CONDITIONAL([BUILD_DEBUGGER_DLL], [test x$BUILD_TVDLL = xyes])

# ----------------------------------------------------------------------------

nameserv_name=""
# Get the default nameserver, if no nameserver was selected.  A process
# manager may advertise a nameserver name by setting the variable
# MPID_PM_NAMESERVER.
if test "$with_namepublisher" = "default" ; then
   if test -n "$MPID_PM_NAMESERVER" ; then
       with_namepublisher=$MPID_PM_NAMESERVER
   else
       # The default is to use pmi to communicate published names
       with_namepublisher=pmi
   fi
fi

if test "$with_namepublisher" != no -a "$with_namepublisher" != "none" ; then
    case "$with_namepublisher" in
    none|no) ;;
    # Removed ldap namepublisher hook - we no longer support or distribute
    # the ldap-based name server
    file*)
    # Note that we always build the Makefile for the file version because
    # this name publisher is really too simple to require a
    # separate configure, and we might as well include a basic
    # name publisher with any MPICH distribution
    # We DO need to extract the directory name that is used for writing
    # the files, with the User's home directory as the default
    nameserv_name="file"
    basedir=`echo $with_namepublisher | sed -e 's/file://'`
    if test "$basedir" = "$with_namepublisher" ; then
        # Reset since no directory was set.
	basedir='"."';
    fi
    AC_DEFINE_UNQUOTED(FILE_NAMEPUB_BASEDIR,$basedir,[Directory to use in namepub])
    AC_DEFINE(USE_FILE_FOR_NAMEPUB,1,[Define if file should be used for name publisher])
    ;;

    *)
    # Check for a new namepublisher
    dir=$with_namepublisher
    # If we later need args, here is where we can strip them off of the
    # with argument
    if test -d "$use_top_srcdir/src/nameserv/$dir" ; then
        if test -x "$use_top_srcdir/src/nameserv/$dir/configure" ; then
	    # Run the configure in this directory if necessary
            subsystems="$subsystems src/nameserv/$dir"
        fi
	nameserv_name=$dir
    else
        AC_MSG_WARN([Unknown name publisher $with_namepublisher])
    fi
    ;;
    esac
fi
if test -n "$nameserv_name" ; then
   AC_DEFINE(HAVE_NAMEPUB_SERVICE,1,[Define if a name publishing service is available])
fi
export nameserv_name
AM_CONDITIONAL([BUILD_NAMEPUB_FILE],[test "X$nameserv_name" = "Xfile"])
AM_CONDITIONAL([BUILD_NAMEPUB_PMI],[test "X$nameserv_name" = "Xpmi"])

# In case the documentation targets are used, find doctext and attempt to
# find the source for the doctext LaTeX style files.  Use "false" if
# doctext is not found
AC_PATH_PROG(DOCTEXT,doctext,false)
export DOCTEXT
AC_SUBST(DOCTEXT)

# ----------------------------------------------------------------------------
# Check for C compiler characteristics
AC_C_CONST
AC_C_VOLATILE
AC_C_RESTRICT
AC_C_INLINE

PAC_C_GNU_ATTRIBUTE
PAC_C_BUILTIN_EXPECT
PAC_C_STATIC_ASSERT

# We need to check for the endianess in order to implement the
# "external32" representations.  This defines "WORDS_BIGENDIAN when
# the system is bigendian.
# As of autoconf 2.62, this macro takes an additional argument for systems
# that can produce object files for either endianess.
# With the as-always-incompatible-with-every-version autoconf, the
# arguments for this macro *changed* in 2.62 to
# (if-bigendian,if-littleendian,unknown,universal)
# The fourth argument is new.
# Also note that the definition emitted by autoheader requires that gcc
# be used to compile the programs - other compilers may not define the
# non-standard __BIG_ENDIAN__ or __LITTLE_ENDIAN__ CPP names on which
# autoconf 2.62 now depends.
byteOrdering=unknown
AC_C_BIGENDIAN(byteOrdering=big,byteOrdering=little,,byteOrdering=universal)
case $byteOrdering in
    big)
    # Nothing to do - the c_bigendian macro takes care of it
    :
    ;;
    little)
    AC_DEFINE(WORDS_LITTLEENDIAN,1,[Define if words are little endian])
    ;;
    universal)
    AC_DEFINE(WORDS_UNIVERSAL_ENDIAN,1,[Define if configure will not tell us, for universal binaries])
    ;;
    unknown)
    AC_MSG_ERROR([Unable to determine endianess])
    ;;
esac

# We only need this test if we are using Fortran
if test "$enable_f77" ; then
    PAC_PROG_C_UNALIGNED_DOUBLES(,
[AC_MSG_WARN(Your C compiler $CC does not support unaligned accesses
to doubles.  This is required for interoperation with
Fortran (the Fortran standard requires it).
You may need to specify an additional argument to your C compiler to
force it to allow unaligned accesses.)])
fi
# Check for __func__ (defined in C99) or __FUNCTION__ (defined in older GCC)
AC_CACHE_CHECK([whether $CC supports __func__],pac_cv_cc_has___func__,
[AC_TRY_COMPILE([],
 [const char *cp = __func__; ],pac_cv_cc_has___func__=yes,
pac_cv_cc_has___func__=no)])
if test "$pac_cv_cc_has___func__" != "yes" ; then
    AC_CACHE_CHECK([whether $CC supports __FUNCTION__],pac_cv_cc_has___FUNCTION__,
[AC_TRY_COMPILE([],
 [const char *cp = __FUNCTION__;],pac_cv_cc_has___FUNCTION__=yes,
pac_cv_cc_has___FUNCTION__=no)])
fi

#------------------------------------------------------- 
# C types

# AC_CHECK_SIZEOF etc. Sets $ac_cv_sizeof_type. Defines SIZEOF_TYPE.
# Unsigned types are assumed to have the same size as signed types.

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(int8_t)
AC_CHECK_SIZEOF(int16_t)
AC_CHECK_SIZEOF(int32_t)
AC_CHECK_SIZEOF(int64_t)

AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_SIZEOF(wchar_t, 0, [
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
])

# Other C99 types.
AC_CHECK_HEADERS([stdbool.h complex.h])
AC_CHECK_SIZEOF([_Bool],0,[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
])
AC_CHECK_SIZEOF([float _Complex],0,[
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif
])
AC_CHECK_SIZEOF([double _Complex],0,[
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif
])
AC_CHECK_SIZEOF([long double _Complex],0,[
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif
])
AC_CHECK_SIZEOF([_Float16])

# sanity check
if test $ac_cv_sizeof_short -eq 0 -o \
        $ac_cv_sizeof_int -eq 0 -o \
        $ac_cv_sizeof_long -eq 0 ; then
        # FIXME: CROSS_SIZEOF_typename mechanism is gone.
	AC_MSG_ERROR([Configure was unable to determine the size of types ; if cross compiling,
use the --with-cross=file configure option to specify a file containing
autoconf cache value, e.g. ac_cv_sizeof_int=4. The perl script
maint/gen_cross.pl can be used to run on the target system to generate
a sample crossfile. Pass this cross file to configure using the
--with-cross=path/to/crossfile option])
fi

# MPI_FINT, MPI_AINT, MPI_OFFSET, MPI_COUNT
MPI_SIZEOF_FINT=$ac_cv_sizeof_int
MPI_SIZEOF_AINT=$ac_cv_sizeof_void_p
MPI_SIZEOF_OFFSET=8
if test $MPI_SIZEOF_AINT -gt $MPI_SIZEOF_OFFSET ; then
    MPI_SIZEOF_COUNT=$MPI_SIZEOF_AINT
else
    MPI_SIZEOF_COUNT=$MPI_SIZEOF_OFFSET
fi

AC_MSG_CHECKING([size of MPI_Fint])
AC_MSG_RESULT([$MPI_SIZEOF_FINT])
AC_MSG_CHECKING([size of MPI_Aint])
AC_MSG_RESULT([$MPI_SIZEOF_AINT])
AC_MSG_CHECKING([size of MPI_Offset])
AC_MSG_RESULT([$MPI_SIZEOF_OFFSET])
AC_MSG_CHECKING([size of MPI_Count])
AC_MSG_RESULT([$MPI_SIZEOF_COUNT])
AC_DEFINE_UNQUOTED([SIZEOF_FINT],[$MPI_SIZEOF_FINT],[The size of MPI_Fint.])
AC_DEFINE_UNQUOTED([SIZEOF_AINT],[$MPI_SIZEOF_AINT],[The size of MPI_Aint.])
AC_DEFINE_UNQUOTED([SIZEOF_OFFSET],[$MPI_SIZEOF_OFFSET],[The size of MPI_Offset.])
AC_DEFINE_UNQUOTED([SIZEOF_COUNT],[$MPI_SIZEOF_COUNT],[The size of MPI_Count.])

# ----------------------------------------------------------------------------
# Get default structure alignment for integers
# ----------------------------------------------------------------------------
dnl PAC_C_MAX_INTEGER_ALIGN places the default alignment into
dnl pac_cv_c_max_integer_align, with possible values including
dnl packed (byte), largest, two, four, eight (or other failure message).
PAC_C_MAX_INTEGER_ALIGN

if test "$pac_cv_c_max_integer_align" = "packed" ; then
    pac_cv_c_struct_align_nr=1
elif test "$pac_cv_c_max_integer_align" = "two" ; then
    pac_cv_c_struct_align_nr=2
elif test "$pac_cv_c_max_integer_align" = "four" ; then
    pac_cv_c_struct_align_nr=4
elif test "$pac_cv_c_max_integer_align" = "eight" ; then
    pac_cv_c_struct_align_nr=8
fi

if test -n "$pac_cv_c_struct_align_nr" ; then
    AC_DEFINE_UNQUOTED(HAVE_MAX_INTEGER_ALIGNMENT,$pac_cv_c_struct_align_nr,[Controls byte alignment of integer structures (for MPI structs)])
    AC_DEFINE_UNQUOTED(HAVE_MAX_STRUCT_ALIGNMENT,$pac_cv_c_struct_align_nr,[Controls byte alignment of structures (for aligning allocated structures)])
fi
# Get default structure alignment for floating point types
dnl PAC_C_MAX_FP_ALIGN places the default alignment into
dnl pac_cv_c_max_fp_align, with possible values including
dnl packed (byte), largest, two, four, eight (or other failure message).
PAC_C_MAX_FP_ALIGN

if test "$pac_cv_c_max_fp_align" = "packed" ; then
    pac_cv_c_fp_align_nr=1
elif test "$pac_cv_c_max_fp_align" = "two" ; then
    pac_cv_c_fp_align_nr=2
elif test "$pac_cv_c_max_fp_align" = "four" ; then
    pac_cv_c_fp_align_nr=4
elif test "$pac_cv_c_max_fp_align" = "eight" ; then
    pac_cv_c_fp_align_nr=8
elif test "$pac_cv_c_max_fp_align" = "sixteen" ; then
    pac_cv_c_fp_align_nr=16
elif test "$pac_cv_c_max_fp_align" = "largest" ; then
    AC_MSG_ERROR([Configure detected unsupported structure alignment rules.])
fi

if test -n "$pac_cv_c_fp_align_nr" ; then
    AC_DEFINE_UNQUOTED(HAVE_MAX_FP_ALIGNMENT,$pac_cv_c_fp_align_nr,[Controls byte alignment of structures with floats, doubles, and long doubles (for MPI structs)])
fi

# Test for the alignment of structs containing only long doubles.
if test "$pac_cv_have_long_double" = yes ; then
    # Check for alignment of just float and double (no long doubles)
    PAC_C_MAX_DOUBLE_FP_ALIGN
    PAC_C_MAX_LONGDOUBLE_FP_ALIGN
    # FIXME: If this alignment is not the same as that for all float types,
    # we need to do something else in the alignment rule code.
    if test "$pac_cv_c_max_fp_align" != "$pac_cv_c_max_longdouble_fp_align" -o \
            "$pac_cv_c_max_fp_align" != "$pac_cv_c_max_double_fp_align" ; then
        AC_MSG_WARN([Structures containing long doubles may be aligned differently from structures with floats or longs.  MPICH does not handle this case automatically and you should avoid assumed extents for structures containing float types.])

	double_align=-1
	case $pac_cv_c_max_double_fp_align in
	packed) double_align=1 ;;
	two)    double_align=2 ;;
	four)   double_align=4 ;;
	eight)  double_align=8 ;;
	esac
	longdouble_align=-1
	case $pac_cv_c_max_longdouble_fp_align in
	packed) longdouble_align=1 ;;
	two)    longdouble_align=2 ;;
	four)   longdouble_align=4 ;;
	eight)  longdouble_align=8 ;;
	sixteen)longdouble_align=16 ;;
	esac

	AC_DEFINE_UNQUOTED(HAVE_MAX_DOUBLE_FP_ALIGNMENT,$double_align,[Controls byte alignment of structs with doubles])
	AC_DEFINE_UNQUOTED(HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT,$longdouble_align,[Controls byte alignment of structs with long doubles])
    fi
fi

# Test for weird struct alignment rules that vary padding based on
# size of leading type only.
PAC_C_DOUBLE_POS_ALIGN
if test "$pac_cv_c_double_pos_align" = "yes" ; then
    AC_DEFINE_UNQUOTED(HAVE_DOUBLE_POS_ALIGNMENT,1,[Controls how alignment is applied based on position of doubles in the structure])
fi

# Test for same weird issues with long long int.
PAC_C_LLINT_POS_ALIGN
if test "$pac_cv_c_llint_pos_align" = "yes" ; then
   AC_DEFINE_UNQUOTED(HAVE_LLINT_POS_ALIGNMENT,1,[Controls how alignment is applied based on position of long long ints in the structure])
fi

# Test for double alignment not following all our other carefully constructed rules
PAC_C_DOUBLE_ALIGNMENT_EXCEPTION
if test "$pac_cv_c_double_alignment_exception" = "four" ; then
   AC_DEFINE_UNQUOTED(HAVE_DOUBLE_ALIGNMENT_EXCEPTION,4,[Controls how alignment of doubles is performed, separate from other FP values])
fi

# Require strict alignment memory access for alignment-sensitive platform (e.g., SPARC)
if test "$host_cpu" = "sparc" ; then
    AC_DEFINE(NEEDS_STRICT_ALIGNMENT,1,[Define if strict alignment memory access is required])
fi

# ----------------------------------------------------------------------------
# We can now create the Fortran versions of the datatype values, along with
# some of the other datatype-dependent sizes

# There are two parts to handling the datatypes:
#    Convert the C datatype values to their Fortran equivalent.  This
#    involves converting the hex values for the C version into decimal
#    since standard Fortran does not have hex constants
#
#    Determine the existence of the Fortran 'sized' types and set those
#    values.
#
# In addition, we need to look at a few additional constants that depend
# on how the compiler sizes some datatypes.  These are:
#    STATUS_SIZE, INTEGER_KIND, ADDRESS_KIND, and OFFSET_KIND
#
# ----------------------------------------------------------------------------
if test "$enable_f77" = yes ; then
    # Up to size checking code for F77 data types.
    # The results are available in pac_cv_sizeof_f77_<type>
    # Size is 0 if unknown or unavailable (or cross-compiling)
    #
    # Cross-compilation results can be included with the --with-cross=file
    # option.
    CROSS_F77_SIZEOF_INTEGER=${CROSS_F77_SIZEOF_INTEGER:-0}
    CROSS_F77_SIZEOF_REAL=${CROSS_F77_SIZEOF_REAL:-0}
    CROSS_F77_SIZEOF_DOUBLE_PRECISION=${CROSS_F77_SIZEOF_DOUBLE_PRECISION:-0}
    PAC_PROG_F77_CHECK_SIZEOF_EXT(integer,$CROSS_F77_SIZEOF_INTEGER)
    PAC_PROG_F77_CHECK_SIZEOF_EXT(real,$CROSS_F77_SIZEOF_REAL)
    PAC_PROG_F77_CHECK_SIZEOF_EXT(double precision,$CROSS_F77_SIZEOF_DOUBLE_PRECISION)
    if test $pac_cv_f77_sizeof_integer -eq 0 -o \
            $pac_cv_f77_sizeof_real -eq 0 ; then
        AC_MSG_ERROR([Unable to configure with Fortran support because configure could not
determine the size of a Fortran INTEGER, REAL, DOUBLE PRECISION. If cross
compiling, use --with-cross=file configure option to specify a file containing
shell assignments to CROSS_F77_SIZEOF_typename, e.g., CROSS_F77_SIZEOF_INTEGER.
The program maint/getcross.pl can be run on the target system to generate this file.
])
    fi

    if test $MPI_SIZEOF_AINT -gt $pac_cv_f77_sizeof_integer ; then
        AC_DEFINE(HAVE_AINT_LARGER_THAN_FINT,1,[Define if addresses are larger than Fortran integers])
    fi
    if test $MPI_SIZEOF_AINT -ne $pac_cv_f77_sizeof_integer ; then
	AC_DEFINE(HAVE_AINT_DIFFERENT_THAN_FINT,1,[Define if addresses are a different size than Fortran integers])
    fi
    if test $pac_cv_f77_sizeof_integer = $ac_cv_sizeof_int ; then
        AC_DEFINE(HAVE_FINT_IS_INT,1,[Define if Fortran integer are the same size as C ints])
    else
        AC_MSG_ERROR([Fortran integers and C ints are not the same size.  The current Fortran binding does not support this case.  Either force the Fortran compiler to use integers of $ac_cv_sizeof_int bytes, or use --disable-fortran on the configure line for MPICH.])
    fi

    PAC_F77_CHECK_TYPE([integer*1])
    PAC_F77_CHECK_TYPE([integer*2])
    PAC_F77_CHECK_TYPE([integer*4])
    PAC_F77_CHECK_TYPE([integer*8])
    PAC_F77_CHECK_TYPE([integer*16])
    PAC_F77_CHECK_TYPE([real*2])
    PAC_F77_CHECK_TYPE([real*4])
    PAC_F77_CHECK_TYPE([real*8])
    PAC_F77_CHECK_TYPE([real*16])

    AC_LANG_C

    # FIXME: do we need it or not?
    # Try and compute the values of .true. and .false. in Fortran
    # This code has been removed because the Fortran binding code does
    # not yet support it.
    PAC_F77_LOGICALS_IN_C([$MPI_FINT])

    # Get the INTEGER_KIND, ADDRESS_KIND and OFFSET_KIND if possible
    #
    # For Fortran 90, we'll also need MPI_ADDRESS_KIND and MPI_OFFSET_KIND
    # Since our compiler might BE a Fortran 90 compiler, try and determine the
    # values.
    if test "$FC" = "no" ; then
        PAC_F77_IS_FC([
            FC=$F77
            if test -z "$FCFLAGS" ; then
                FCFLAGS="$FFLAGS"
            fi
        ])
    fi
    if test "$FC" != "no" ; then
        PAC_PROG_FC_INT_KIND(ADDRESS_KIND,$MPI_SIZEOF_AINT,$CROSS_F90_ADDRESS_KIND)
        if test "$MPI_SIZEOF_AINT" = 8 ; then
            OFFSET_KIND=$ADDRESS_KIND
        else
            PAC_PROG_FC_INT_KIND(OFFSET_KIND,8,$CROSS_F90_OFFSET_KIND)
        fi
        #
        PAC_PROG_FC_INT_KIND(INTEGER_KIND,$pac_cv_f77_sizeof_integer,$CROSS_F90_INTEGER_KIND)
        if test "$INTEGER_KIND" = "-1" ; then
	    # In our experience, this usually means that there is some
	    # problem building and/or running the f90 program.  Fail
	    # in this case rather than attempt to continue
	    AC_MSG_ERROR([Unable to determine Fortran 90 KIND values for either address-sized integers or offset-sized integers.])
        fi
	#
        # Some compilers won't allow a -1 kind (e.g., absoft).  In this case,
        # use a fallback (sizeof(int) kind)
        if test "$ADDRESS_KIND" = "-1" -o "$OFFSET_KIND" = "-1" ; then
            if test "$ADDRESS_KIND" = "-1" ; then
	        ADDRESS_KIND=$INTEGER_KIND
            fi
            if test "$OFFSET_KIND" = "-1" ; then
	        OFFSET_KIND=$INTEGER_KIND
            fi
        fi
	AC_LANG_PUSH([Fortran])
        AC_CACHE_CHECK([if real*8 is supported in Fortran 90],
                       [pac_cv_fort90_real8],[
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([],[      real*8 a])
            ],[
                pac_cv_fort90_real8=yes
            ],[
                pac_cv_fort90_real8=no
            ])
        ])
	AC_LANG_POP([Fortran])
	WTIME_DOUBLE_TYPE="DOUBLE PRECISION"
	# Save a copy of the original mpi_base.f90 file
	if test "$enable_fc" = "yes" -a "$pac_cv_fort90_real8" = "yes" ; then
	    WTIME_DOUBLE_TYPE="REAL*8"
        fi
	# WTIME_DOUBLE_TYPE is substituted into mpi_base.f90
	AC_SUBST(WTIME_DOUBLE_TYPE)
    fi
    # Make sure that address kind and offset kind have values.
    if test -z "$ADDRESS_KIND" ; then
        ADDRESS_KIND=0
    fi
    if test -z "$OFFSET_KIND" ; then
        OFFSET_KIND=0
    fi
    # Note, however, that zero value are (in all practical case) invalid
    # for Fortran 90, and indicate a failure.  Test and fail if Fortran 90
    # enabled.
    if test "$enable_fc" = "yes" ; then
        if test "$ADDRESS_KIND" -le 0 -o "$OFFSET_KIND" -le 0 ; then
	    AC_MSG_ERROR([Unable to determine Fortran 90 integer kinds for MPI types.  If you do not need Fortran 90, add --disable-fc to the configure options.])
	    # If the above is converted to a warning, you need to change
	    # enable_fc and remote f90 from the bindings
	    enable_fc=no
        fi
    fi

    AS_IF([test "$MPI_SIZEOF_AINT" -gt "$MPI_SIZEOF_OFFSET"],
      [ COUNT_KIND="$ADDRESS_KIND"],
      [ COUNT_KIND="$OFFSET_KIND" ])

    AC_SUBST(ADDRESS_KIND)
    AC_SUBST(OFFSET_KIND)
    AC_SUBST(INTEGER_KIND)
    AC_SUBST([COUNT_KIND])

    # Some compilers may require special directives to handle the common
    # block in a library.  In particular, directives are needed for Microsoft
    # Windows to support dynamic library import.  The following six
    # directives may be needed:
    #  CMS\$ATTRIBUTES DLLIMPORT::/MPIPRIV1/
    #  CMS\$ATTRIBUTES DLLIMPORT::/MPIPRIV2/
    #  CMS\$ATTRIBUTES DLLIMPORT::/MPIPRIVC/
    #  CDEC\$ATTRIBUTES DLLIMPORT::/MPIPRIV1/
    #  CDEC\$ATTRIBUTES DLLIMPORT::/MPIPRIV2/
    #  CDEC\$ATTRIBUTES DLLIMPORT::/MPIPRIVC/
    # CMS is for the Microsoft compiler,
    # CDEC is (we believe) for the DEC Fortran compiler.
    # We need to make this a configure-time variable because some compilers
    # (in particular, a version of the Intel Fortran compiler for Linux)
    # will read directives for other compilers and then flag as fatal
    # errors directives that it does not support but does recognize.

    DLLIMPORT=""
    AC_SUBST(DLLIMPORT)

    # If other "kinds" are supported, MPI_SIZEOF needs to identify
    # those as well.  This is very difficult to do in a general way.

    # To start with, we use the sizes determined for the Fortran 77 values.
    # These must be the same as for the Fortran 90 values.
    CROSS_F90_SIZEOF_INTEGER=${CROSS_F90_SIZEOF_INTEGER:-0}
    CROSS_F90_SIZEOF_REAL=${CROSS_F90_SIZEOF_REAL:-0}
    CROSS_F90_SIZEOF_DOUBLE_PRECISION=${CROSS_F90_SIZEOF_DOUBLE_PRECISION:-0}
    SIZEOF_FC_INTEGER=$CROSS_F90_SIZEOF_INTEGER
    SIZEOF_FC_REAL=$CROSS_F90_SIZEOF_REAL
    SIZEOF_FC_CHARACTER=1
    SIZEOF_FC_DOUBLE_PRECISION=$CROSS_F90_SIZEOF_DOUBLE_PRECISION
    if test "$pac_cv_f77_sizeof_integer" -gt 0 -a \
	    "$SIZEOF_FC_INTEGER" = "0" ; then
        SIZEOF_FC_INTEGER=$pac_cv_f77_sizeof_integer
    fi
    if test "$pac_cv_f77_sizeof_real" -gt 0 -a "$SIZEOF_FC_REAL" = "0" ; then
        SIZEOF_FC_REAL=$pac_cv_f77_sizeof_real
    fi
    if test "$pac_cv_f77_sizeof_double_precision" -gt 0 -a \
       "$SIZEOF_FC_DOUBLE_PRECISION" = "0" ; then
        SIZEOF_FC_DOUBLE_PRECISION=$pac_cv_f77_sizeof_double_precision
    fi
    AC_SUBST(SIZEOF_FC_INTEGER)
    AC_SUBST(SIZEOF_FC_REAL)
    AC_SUBST(SIZEOF_FC_DOUBLE_PRECISION)
    AC_SUBST(SIZEOF_FC_CHARACTER)

    # REQD is short for "real equal double precision" and is set to the
    # Fortran 90 comment character if true.  This is necessary to
    # allow the mpi_sizeofs module to be built, since if this part of the
    # Fortran standard is violated by the compiler (unfortunately common,
    # as some applications are written to require this non-standard
    # version), the double precision versions of the MPI_SIZEOF routine
    # must be commented out of the module (!).
    REQD=
    if test "$SIZEOF_FC_REAL" = "$SIZEOF_FC_DOUBLE_PRECISION" ; then
        REQD="!"
    fi
    AC_SUBST(REQD)
    # Is integer*1 supported, and is it a different size than integer?
    REQI1="!"
    if test "$pac_cv_fort_integer1" = yes -a "$SIZEOF_FC_INTEGER" != 1 ; then
        REQI1=
    fi
    AC_SUBST(REQI1)
    # Is integer*2 supported, and is it a different size than integer?
    REQI2="!"
    if test "$pac_cv_fort_integer2" = yes -a "$SIZEOF_FC_INTEGER" != 2 ; then
        REQI2=
    fi
    AC_SUBST(REQI2)
    # Is integer*8 supported, and is it a different size than integer?
    REQI8="!"
    if test "$pac_cv_fort_integer8" = yes -a "$SIZEOF_FC_INTEGER" != 8 ; then
        REQI8=
    fi
    AC_SUBST(REQI8)
    #

    AC_LANG_C
fi

# ----------------------------------------------------------------------------
# CXX types

if test "$enable_cxx" = "yes" ; then
    AC_LANG_CPLUSPLUS
    AC_CHECK_HEADER(complex)
    if test "$ac_cv_header_complex" != "yes" ; then
        ac_cv_sizeof_Complex=0
        ac_cv_sizeof_DoubleComplex=0
        ac_cv_sizeof_LongDoubleComplex=0
    fi
    if test "$ac_cv_sizeof_long_double" = 0 ; then
        ac_cv_sizeof_LongDoubleComplex=0
    fi

    # bool, Complex, DoubleComplex, LongDoubleComplex
    AC_CHECK_SIZEOF(bool)

    AC_CHECK_SIZEOF(FloatComplex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define FloatComplex complex<float>
])
    AC_CHECK_SIZEOF(DoubleComplex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define DoubleComplex complex<double>
])
    AC_CHECK_SIZEOF(LongDoubleComplex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define LongDoubleComplex complex<long double>
])

    AC_LANG_C
fi

# ------------------------------------------------------------------------
# Test if  type_tag_for_datatype is agnostic to modifiers such as const, volatile, and restrict
# if not, the type tags are disabled to avoid compiler warings. A new type_tag for const etc. does
# does not help, since only the latest type_tag definition is used. Type tags are defined in mpi.h,
# therefore, they must be also be activated/deavtivated there
PAC_PUSH_FLAG([CFLAGS])
PAC_APPEND_FLAG([-Werror],[CFLAGS])
AC_TRY_COMPILE([
typedef int TEST_Datatype;
#define TEST_INT ((TEST_Datatype)0x44)
static const TEST_Datatype test_int  __attribute__((type_tag_for_datatype(TEST,int)))  = TEST_INT;
void test(const void* buffer, TEST_Datatype datatype)__attribute__((pointer_with_type_tag(TEST,1,2)));],[
const int buf[10];
test(buf, TEST_INT);],ac_attr_support=yes, ac_attr_support=no)
if test "$ac_attr_support" = yes
then
    DISABLE_TAG_SUPPORT="#undef NO_TAGS_WITH_MODIFIERS"
else
    DISABLE_TAG_SUPPORT="#define NO_TAGS_WITH_MODIFIERS 1"
fi
PAC_POP_FLAG([CFLAGS])
AC_SUBST(DISABLE_TAG_SUPPORT)

# ----------------------------------------------------------------------------
# Check for the alignment rules moves with types int64_t etc.  These
# are used in the datatype code to perform pack and unpack operations.
# These only determine if different alignments *work*, not whether they
# work efficiently.  The datatype pack code (should) allow the developer
# to include stricter alignment rules than are needed for correctness to
# get better performance.
if test "$ac_cv_c_int64_t" != "no" -o -n "$INT64_T" ; then
    default_int64_t_alignment=${CROSS_INT64_T_ALIGNMENT:-"unknown"}
    if test -z "$INT64_T" ; then
         if test "$ac_cv_c_int64_t" = yes ; then
	     INT64_T="int64_t"
         else
             INT64_T="$ac_cv_int64_t"
         fi
    fi
    # We use the type that we're going use for int64.
    AC_CACHE_CHECK([for alignment restrictions on $INT64_T],pac_cv_int64_t_alignment,[
    AC_TRY_RUN([
#include <sys/types.h>
#include <stdlib.h>
int main(int argc, char **argv )
{
    $INT64_T *p1, v;
    char    *buf_p = (char *)malloc( 64 ), *bp;
    bp = buf_p;
    /* Make bp aligned on 4, not 8 bytes */
    if (!( (long)bp & 0x7 ) ) bp += 4;
    p1 = ($INT64_T *)bp;
    v = -1;
    *p1 = v;
    if (!( (long)bp & 0x3 ) ) bp += 2;
    p1 = ($INT64_T *)bp;
    *p1 = 1;
    if (!( (long)bp & 0x1 ) ) bp += 1;
    p1 = ($INT64_T *)bp;
    *p1 = 1;
    return 0;
}
],pac_cv_int64_t_alignment=no,pac_cv_int64_t_alignment=yes,pac_cv_int64_t_alignment=$default_int64_t_alignment)
])
    if test "$pac_cv_int64_t_alignment" = "no" ; then
         AC_DEFINE(HAVE_ANY_INT64_T_ALIGNMENT,1,[Define if int64_t works with any alignment])
    fi
fi

if test "$ac_cv_int32_t" != "no" ; then
    default_int32_t_alignment=${CROSS_INT32_T_ALIGNMENT:-"unknown"}
    if test -z "$INT32_T" ; then
         if test "$ac_cv_c_int32_t" = yes ; then
	     INT32_T="int32_t"
         else
             INT32_T="$ac_cv_int32_t"
         fi
    fi

    AC_CACHE_CHECK([for alignment restrictions on int32_t],pac_cv_int32_t_alignment,[
    AC_TRY_RUN([
#include <sys/types.h>
#include <stdlib.h>
int main(int argc, char **argv )
{
    $INT32_T *p1, v;
    char    *buf_p = (char *)malloc( 64 ), *bp;
    bp = buf_p;
    /* Make bp aligned on 4, not 8 bytes */
    if (!( (long)bp & 0x7 ) ) bp += 4;
    p1 = ($INT32_T *)bp;
    v = -1;
    *p1 = v;
    if (!( (long)bp & 0x3 ) ) bp += 2;
    p1 = ($INT32_T *)bp;
    *p1 = 1;
    if (!( (long)bp & 0x1 ) ) bp += 1;
    p1 = ($INT32_T *)bp;
    *p1 = 1;
    return 0;
}
],pac_cv_int32_t_alignment=no,pac_cv_int32_t_alignment=yes,pac_cv_int32_t_alignment=$default_int32_t_alignment)
])
    if test "$pac_cv_int32_t_alignment" = "no" ; then
         AC_DEFINE(HAVE_ANY_INT32_T_ALIGNMENT,1,[Define if int32_t works with any alignment])
    fi
fi

# -----------------------------------------------------------------------------
# Check for support of enable-coverage.  Put this near the end of the tests
# because the coverage options may affect the other tests.
PAC_ENABLE_COVERAGE

# -----------------------------------------------------------------------------
# Look for Standard headers
AC_HEADER_STDC
# Check for a specific header
# Grrr.  OS/X fails the test for sys/uio.h because uio *requires* sys/types.h
# to compile.  Thus, we'll make that a separate test
# stddef.h is sometimes needed for types like wchar_t
AC_CHECK_HEADERS(stdlib.h stdarg.h sys/types.h string.h inttypes.h limits.h stddef.h errno.h sys/socket.h sys/time.h unistd.h endian.h assert.h sys/param.h)
AC_CACHE_CHECK([for sys/uio.h],ac_cv_header_sys_uio_h,[
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/uio.h>
],[int a;],ac_cv_header_sys_uio_h=yes,ac_cv_header_sys_uio_h=no)])
if test "$ac_cv_header_sys_uio_h" = yes ; then
    AC_DEFINE(HAVE_SYS_UIO_H,1,[Define if you have the <sys/uio.h> header file.])
fi

# Check for special types
AC_TYPE_SIZE_T

# These are used to support timeouts
AC_CHECK_FUNCS(setitimer alarm)
# These are used for error reporting
AC_CHECK_FUNCS(vsnprintf vsprintf)
if test "$ac_cv_func_vsnprintf" = "yes" ; then
    # vsnprintf may be declared in stdio.h and may need stdarg.h
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>
#include <stdarg.h>],vsnprintf)
fi
# We would like to use strerror in the file namepublisher; it is also used
# in MPIR_Strerror (whose implementation is broken if strerror is not found)
AC_CHECK_FUNCS(strerror strncasecmp)
AC_FUNC_STRERROR_R
if test "$ac_cv_func_strerror_r" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strerror_r)
fi

# Use snprintf if possible when creating messages
AC_CHECK_FUNCS(snprintf)
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi

# qsort will be used in MPI_Comm_split, if available
AC_CHECK_FUNCS([qsort])

# if we are using stdarg, we may need va_copy .  Test to see if we have it
# Since it may be a built-in instead of a function, we must try to
# compile and link a program that uses it.
# va_copy is currently used only in src/util/dbg_printf.c, in an obsolete
# debugging routine.  We may want to withdraw this (saving the
# test in confdb/aclocal_cc.m4).
AC_CACHE_CHECK([for va_copy],pac_cv_func_va_copy,[
AC_TRY_LINK([
#include <stdarg.h>
void foo1( char *fmt, ... ) {
    va_list ap, list;
    va_start(ap,fmt);
    va_copy(list,ap);
    va_end(list);
    va_end(ap);
}
],[foo1("a test %d", 3);],pac_cv_func_va_copy=yes,pac_cv_func_va_copy=no)])

if test "$pac_cv_func_va_copy" = "yes" ; then
    AC_DEFINE(HAVE_VA_COPY,1,[Define if we have va_copy])
else
    AC_CACHE_CHECK([for __va_copy],pac_cv_func___va_copy,[
    AC_TRY_LINK([
#include <stdarg.h>
void foo1( char *fmt, ... ) {
    va_list ap, list;
    va_start(ap,fmt);
    __va_copy(list,ap);
    va_end(list);
    va_end(ap);
}
],[foo1("a test %d", 3);],pac_cv_func___va_copy=yes,pac_cv_func___va_copy=no)])
    if test "$pac_cv_func___va_copy" = "yes" ; then
        AC_DEFINE(HAVE___VA_COPY,1,[Define if we have __va_copy])
    fi
fi

PAC_C_MACRO_VA_ARGS

# Check for alloca function.  May set HAVE_ALLOCA_H and HAVE_ALLOCA
AC_FUNC_ALLOCA
# We don't use alloca unless USE_ALLOCA is also set.
AC_ARG_ENABLE(alloca,
	AC_HELP_STRING([--enable-alloca],
		[Use alloca to allocate temporary memory if available]),,enable_alloca=no)
if test "$enable_alloca" = yes ; then
    AC_DEFINE(USE_ALLOCA,1,[Define if alloca should be used if available])
fi

if test "$enable_g_mem" != "yes" ; then
    # Strdup is needed only if memory tracing is not enabled.
    AC_CHECK_FUNCS(strdup)
    if test "$ac_cv_func_strdup" = "yes" ; then
        # Do we need to declare strdup?
        PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
    fi
fi

# ----------------------------------------------------------------------------
# Look for some non-posix, but commonly provided functions
# ----------------------------------------------------------------------------
# mkstemp() is a better replacement for mktemp()
AC_HAVE_FUNCS(mkstemp)
if test "$ac_cv_func_mkstemp" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],mkstemp)
fi
# putenv() sets environment variable
AC_HAVE_FUNCS(putenv)
if test "$ac_cv_func_putenv" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],putenv)
fi


# ----------------------------------------------------------------------------
# Setup other replaceable values
AC_SUBST(MPILIBNAME)
AC_SUBST(PMPILIBNAME)

if test "$NEEDSPLIB" = "yes" ; then
   LPMPILIBNAME="-l${PMPILIBNAME}"
fi
AC_SUBST(LPMPILIBNAME)

# ----------------------------------------------------------------------------
if test "$ac_cv_sizeof_void_p" -lt "$MPI_SIZEOF_AINT" ; then
    AC_MSG_ERROR([The size of MPI_Aint greater than the size of void * is no longer supported.])
fi

# We need define MPI_Aint to find the size of MPII_Bsend_data_t, but we just need a type of matching size to work.
if test $MPI_SIZEOF_AINT = 4 ; then
    MPI_AINT=int32_t
elif test $MPI_SIZEOF_AINT = 8 ; then
    MPI_AINT=int64_t
fi
AC_CHECK_SIZEOF(MPII_Bsend_data_t,0,[
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#define MPI_Datatype int
typedef $MPI_AINT MPI_Aint;
#include "${master_top_srcdir}/src/include/mpii_bsend.h"]
)
if test "$ac_cv_sizeof_MPII_Bsend_data_t" = "0" ; then
    AC_MSG_ERROR([Unable to determine the size of MPI_BSEND_OVERHEAD])
    # In the past, a default of 128 was used (still likely good enough),
    # but the autoconf SIZEOF macro has been changed to ignore the second
    # argument, so code that depended on the prior defined behavior now
    # silently breaks.
fi
BSEND_OVERHEAD=$ac_cv_sizeof_MPII_Bsend_data_t
export BSEND_OVERHEAD
AC_SUBST(BSEND_OVERHEAD)

dnl Configure any subdirectories.  Note that config.status will *not*
dnl reexecute these!
dnl
dnl Gastly problem.  CONFIG_SUBDIRS only adds the directories to the
dnl list of directories to be configured.  It does NOT control the
dnl timing of the configuration.  For that, we must do something different.
dnl Our original solution was to use a separate macro that does cause
dnl immediate configure; this macro made use of the code that autoconf
dnl uses to handle the subdir configure.  However, later versions of
dnl autoconf did this in a way that caused problems, paritcularly with
dnl errors reported as inconsistent cache files.  Instead, we simply
dnl invoke the configure scripts (if present) directly.

# Check for the Linux functions for controlling processor affinity.
# LINUX: sched_setaffinity
# AIX:   bindprocessor
# OSX (Leopard): thread_policy_set
AC_CHECK_FUNCS(sched_setaffinity sched_getaffinity bindprocessor thread_policy_set)
if test "$ac_cv_func_sched_setaffinity" = "yes" ; then
    # Test for the cpu process set type
    AC_CACHE_CHECK([whether cpu_set_t available],pac_cv_have_cpu_set_t,[
    AC_TRY_COMPILE( [
#include <sched.h>],[ cpu_set_t t; ],pac_cv_have_cpu_set_t=yes,pac_cv_have_cpu_set_t=no)])
    if test "$pac_cv_have_cpu_set_t" = yes ; then
        AC_DEFINE(HAVE_CPU_SET_T,1,[Define if cpu_set_t is defined in sched.h])

	AC_CACHE_CHECK([whether the CPU_SET and CPU_ZERO macros are defined],
	pac_cv_cpu_set_defined,[
        AC_TRY_LINK( [
#include <sched.h>],[ cpu_set_t t; CPU_ZERO(&t); CPU_SET(1,&t); ],
        pac_cv_cpu_set_defined=yes,pac_cv_cpu_set_defined=no)])
	if test "$pac_cv_cpu_set_defined" = "yes" ; then
	    AC_DEFINE(HAVE_CPU_SET_MACROS,1,[Define if CPU_SET and CPU_ZERO defined])
        fi
	# FIXME: Some versions of sched_setaffinity return ENOSYS (!),
	# so we should test for the unfriendly and useless behavior
    fi
fi
if test "$ac_cv_func_thread_policy_set" = yes ; then
    AC_CACHE_CHECK([whether thread affinity macros defined],
    pac_cv_have_thread_affinity_policy,[
    AC_TRY_COMPILE([#include <mach/thread_policy.h>],[
#if !defined(THREAD_AFFINITY_POLICY) || !defined(THREAD_AFFINITY_TAG_NULL)
    :'thread macros not defined
],pac_cv_have_thread_affinity_policy=yes,
  pac_cv_have_thread_affinity_policy=no)])
    if test "$pac_cv_have_thread_affinity_policy" = yes ; then
        AC_DEFINE(HAVE_OSX_THREAD_AFFINITY,1,[Define is the OSX thread affinity policy macros defined])
    fi
fi

# -----------------------------------------------------------------------------
# End of thread configure
# -----------------------------------------------------------------------------


# Checkpointing
AC_ARG_ENABLE(checkpointing,
    [AC_HELP_STRING([--enable-checkpointing], [Enable application checkpointing])],
    [ if test "$enableval" != "no" ; then
	PAC_PUSH_FLAG([LIBS])
	PAC_CHECK_HEADER_LIB_FATAL(blcr, libcr.h, cr, cr_init)
	PAC_APPEND_FLAG([-lcr],[WRAPPER_LIBS])
	PAC_POP_FLAG([LIBS])
        AC_DEFINE(ENABLE_CHECKPOINTING,1,[Application checkpointing enabled])
      fi ],
)

# Update the cache first with the results of the previous configure steps
# We don't use the subdir cache because ensuring that the cache is consistant
# with the way in which configure wishes to use it is very difficult and
# too prone to error.
dnl PAC_SUBDIR_CACHE(always)
# -----------------------------------------------------------------------------
# experiment with creating a cache file
#if test "$CONF_USE_CACHEFILE" = yes ; then
#    basecachefile=`pwd`/cache.base
#    set | grep ac_cv > $basecachefile
#    # Tell other configures to load this file
#    echo "Creating and exporting the base cache file $basecachefile"
#    CONF_BASE_CACHEFILE=$basecachefile
#    export CONF_BASE_CACHEFILE
#fi
PAC_CREATE_BASE_CACHE
# -----------------------------------------------------------------------------

#
# Configure the device second.  This way, any libraries or variables that
# it needs can be extracted before building the other subsystems (this is
# particularly important for the bindings, which may need to know the
# libraries in order to create the compilation scripts)

user_specified_atomic_primitives=no
if test "$DEBUG_SUBDIR_CACHE" = yes ; then
    set -x
fi

dnl "configure" the device here via subconfigure.m4 files
m4_map([PAC_SUBCFG_CONFIGURE_SUBSYS], [PAC_SUBCFG_MODULE_LIST])

# now configure any actual recursively configures subsystems, such as ROMIO and
# hydra, or older components that haven't been updated to a subconfigure.m4 yet
for subsys in $devsubsystems $subsystems ; do
    PAC_CONFIG_SUBDIR([$subsys],[],[AC_MSG_ERROR([$subsys configure failed])])
done
if test "$DEBUG_SUBDIR_CACHE" = yes -a "$enable_echo" != yes ; then
    set +x
fi
dnl PAC_SUBDIR_CACHE_CLEANUP

# Make subsystems available to makefiles.
# FIXME does the makefile actually need this?
subsystems="$devsubsystems $subsystems $bindingsubsystems"

# Find the size of OPA_ptr_t. This step needs to come after the OPA
# configure above in order to get the size OPA_ptr_t evaluated for
# this platform.
AC_CHECK_SIZEOF(OPA_ptr_t,0,[
#include "${master_top_srcdir}/src/openpa/src/opa_primitives.h"
pthread_mutex_t *OPA_emulation_lock;
])

if test $enable_f77 = yes ; then
    # Fortran type for an Offset type (needed to define MPI_DISPLACEMENT_CURRENT
    # The value for this comes from ROMIO, and is needed in mpif.h.in
    # First, we check that this works with both Fortran compilers (if
    # they are defined)
    #
    if test $MPI_SIZEOF_OFFSET = $pac_cv_f77_sizeof_integer ; then
        FORTRAN_MPI_OFFSET=INTEGER
    elif test $MPI_SIZEOF_OFFSET = 8 ; then
        FORTRAN_MPI_OFFSET=INTEGER*8
    else
        AC_MSG_ERROR([Failed to find a FORTRAN integer type for MPI_Offset])
    fi
fi

if test "$enable_f77" = yes -a "$enable_fc" = yes ; then
    AC_LANG_PUSH([Fortran 77])
    AC_MSG_CHECKING([whether the Fortran Offset type works with Fortran 77])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([],[      $FORTRAN_MPI_OFFSET i])
    ],[has_f77_offsettype=yes],[has_f77_offsetype=no])
    AC_MSG_RESULT($has_f77_offsettype)
    AC_LANG_POP([Fortran 77])

    AC_LANG_PUSH([Fortran])
    AC_MSG_CHECKING([whether the Fortran Offset type works with Fortran 90])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([],[      $FORTRAN_MPI_OFFSET i])
    ],[has_fc_offsettype=yes],[has_fc_offsetype=no])
    AC_LANG_POP([Fortran])
    AC_MSG_RESULT($has_fc_offsettype)

    if test "$has_f77_offsettype" != yes -o "$has_fc_offsettype" != yes ; then
        AC_MSG_WARN([mpif.h is not compatible with both $F77 $FFLAGS and $FC $FCFLAGS.  We recommend that you set both F77 and FC to the same compiler and reconfigure.])
    fi
fi
AC_SUBST(FORTRAN_MPI_OFFSET)
#

# ----------------------------------------------------------------------------

#
# Set size of MPI_Status.  Must come after MPI_Count determination.
#

#
# The size of MPI_Status is needed for the Fortran interface.
#
# WARNING!!! this is a spot where we duplicate code from mpi.h.in and it *must*
# be kept in sync in order to make a proper computation
AC_CACHE_CHECK([for size of MPI_Status],[pac_cv_sizeof_mpi_status],[
dnl "double-quote" (in m4), otherwise you end up with a heinous bug because of
dnl the array subscripting below
[
    rm -f pac_mpi_status.h
    cat > pac_mpi_status.h <<_EOF
typedef struct {
    int count_lo;
    int count_hi_and_cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} MPI_Status;
_EOF
]

dnl just compute it, since a 1s-complement or sign-and-magnitude machine is
dnl *highly* unlikely.  Users will report the error if it is ever
dnl encountered, which will be safer than attempting some never-tested
dnl default fallback.
    AC_COMPUTE_INT([pac_cv_sizeof_mpi_status],
                   [sizeof(MPI_Status)],
                   [#include "pac_mpi_status.h"],
                   [AC_MSG_ERROR([unable to compute status size, are you compiling on a non-2s-complement host?])])
    rm -f pac_mpi_status.h
])

SIZEOF_MPI_STATUS=$pac_cv_sizeof_mpi_status
export SIZEOF_MPI_STATUS
AC_SUBST([SIZEOF_MPI_STATUS])

if test "$enable_f77" = yes -a -z "$MPI_STATUS_SIZE" ; then
    if test -n "$SIZEOF_MPI_STATUS" ; then
        # compute from the C sizeof
        AS_VAR_ARITH([MPI_STATUS_SIZE],[$SIZEOF_MPI_STATUS / $ac_cv_sizeof_int])
        if test "$MPI_STATUS_SIZE" = "0" ; then
            AC_MSG_ERROR([Could not compute the size of MPI_Status])
        fi
    else
        AC_MSG_ERROR([MPI_STATUS_SIZE was not defined!])
    fi
fi # enable_f77 and mpi_status_size not set
AC_SUBST([MPI_STATUS_SIZE])
MPIF_STATUS_SIZE=$MPI_STATUS_SIZE
AC_DEFINE_UNQUOTED([MPIF_STATUS_SIZE],[$MPIF_STATUS_SIZE],[Size of an MPI_STATUS, in Fortran, in Fortran integers])

if test "$enable_f77" = yes ; then
    # Check if multiple __attribute__((alias)) is available
    # This test requires MPI_STATUS_SIZE, and thus must be made after
    # MPI_STATUS_SIZE is determined
    if test "$enable_multi_aliases" = "yes" ; then
        PAC_C_MULTI_ATTR_ALIAS
        if test "$pac_c_multi_attr_alias" = "yes" ; then
            PAC_F2C_ATTR_ALIGNED_SIZE([1],[CMB_1INT_ALIGNMENT])
            AC_SUBST(CMB_1INT_ALIGNMENT)
            PAC_F2C_ATTR_ALIGNED_SIZE([$MPI_STATUS_SIZE],[CMB_STATUS_ALIGNMENT],
                                      [32])
            AC_SUBST(CMB_STATUS_ALIGNMENT)
            if test "X$CMB_1INT_ALIGNMENT" != "X" \
                 -a "X$CMB_STATUS_ALIGNMENT" != "X" ; then
                AC_DEFINE(HAVE_C_MULTI_ATTR_ALIAS, 1,
                    [Define if multiple __attribute__((alias)) are supported])
            fi
        fi
    fi
fi

# ----------------------------------------------------------------------------

dnl FIXME XXX DJG does this need to actually be conditional on something?
dnl previously it was conditional on the successful execution of
dnl "src/binding/fortran/use_mpi/configure" but that would only fail if FC couldn't be
dnl determined or a bad VPATH build env was detected
AC_DEFINE(HAVE_FC_TYPE_ROUTINES,1,[Define if Fortran 90 type routines available])

# -----------------------------------------------------------------------------
# Get the value of MPI_MAX_PROCESSOR_NAME
if test -z "$MPID_MAX_PROCESSOR_NAME" ; then
    if test -z "$MPI_MAX_PROCESSOR_NAME" ; then
       MPI_MAX_PROCESSOR_NAME=128
    fi
    AC_MSG_WARN([The $with_device device did not set the maximum size of a processor name, $MPI_MAX_PROCESSOR_NAME being used.])
else
    MPI_MAX_PROCESSOR_NAME=$MPID_MAX_PROCESSOR_NAME
fi
AC_SUBST(MPI_MAX_PROCESSOR_NAME)

# Get the value of MPI_MAX_LIBRARY_VERSION_STRING
if test -z "$MPID_MAX_LIBRARY_VERSION_STRING" ; then
    MPI_MAX_LIBRARY_VERSION_STRING=8192
else
    MPI_MAX_LIBRARY_VERSION_STRING=$MPID_MAX_LIBRARY_VERSION_STRING
fi
AC_SUBST(MPI_MAX_LIBRARY_VERSION_STRING)

# Get the value of MPI_MAX_ERROR_STRING
if test -z "$MPID_MAX_ERROR_STRING" ; then
    if test -z "$MPI_MAX_ERROR_STRING" ; then
       MPI_MAX_ERROR_STRING=512
    fi
    AC_MSG_WARN([The $with_device device did not set the maximum size of an error string, $MPI_MAX_ERROR_STRING being used.])
else
    MPI_MAX_ERROR_STRING=$MPID_MAX_ERROR_STRING
fi
AC_SUBST(MPI_MAX_ERROR_STRING)

# Add a definition that is used to help support Windows DLLs (used in mpi.h.in)
MPIU_DLL_SPEC_DEF="#define MPIU_DLL_SPEC"
AC_SUBST(MPIU_DLL_SPEC_DEF)

dnl We can configure the test directory after the rest of the configure
dnl steps because it does not depend on them.
# set and export values that the test/mpi configure will reference to ensure
# that the correct decisions are made since this configure happens before the
# MPICH library is built.
MPICH_ENABLE_CXX=$enable_cxx
MPICH_ENABLE_F77=$enable_f77
MPICH_ENABLE_FC=$enable_fc
export MPICH_ENABLE_CXX
export MPICH_ENABLE_F77
export MPICH_ENABLE_FC
AM_CONDITIONAL([BUILD_CXX_BINDING],[test "$enable_cxx" = "yes"])
AM_CONDITIONAL([BUILD_F77_BINDING],[test "$enable_f77" = "yes"])
dnl FIXME DJG this has been moved to the f90 bindings subconfigure.m4 for now
dnl AM_CONDITIONAL([BUILD_FC_BINDING],[test "$enable_fc" = "yes"])
# MPI_SRCDIR gives the test/mpi configure the location of the source
# files for an MPI implementation
if test -n "$ac_abs_srcdir" ; then
    MPI_SRCDIR=$ac_abs_srcdir
fi
# Pass these two values with "MPI" names to the test configure
MPI_NO_SPAWN=$MPID_NO_SPAWN
export MPI_NO_SPAWN
MPI_NO_RMA=$MPID_NO_RMA
export MPI_NO_RMA

# Attach program prefix and suffix to executable names for Makefile
AC_SUBST(MPICC_NAME)
AC_SUBST(MPICXX_NAME)
AC_SUBST(MPICPP_NAME)
AC_SUBST(MPIFORT_NAME)
AC_SUBST(MPIF90_NAME)
AC_SUBST(MPIF77_NAME)
PAC_GET_EXENAME("mpicc", MPICC_NAME)
PAC_GET_EXENAME("mpicxx", MPICXX_NAME)
PAC_GET_EXENAME("mpic++", MPICPP_NAME)
PAC_GET_EXENAME("mpifort", MPIFORT_NAME)
PAC_GET_EXENAME("mpif90", MPIF90_NAME)
PAC_GET_EXENAME("mpif77", MPIF77_NAME)

AC_CONFIG_SUBDIRS([test/mpi])
dnl
dnl Generate the Makefiles from Makefile.in
dnl Also generate mpi.h from mpi.h.in so that we can eliminate all ifdefs
dnl from the file.
dnl
dnl Run a setup command for any external modules (normally, this is empty)
dnl Pass a subset of the environment to the invoked process.
AC_OUTPUT_COMMANDS([
for prog in $EXTERNAL_SETUPS - ; do
    if test "$prog" != "-" ; then
      dir=`dirname $prog`
      name=`basename $prog`
      (cd $dir && ./$name)
    fi
done],[
EXTERNAL_SETUPS="$EXTERNAL_SETUPS"
LIBDIR="$libbuild_dir"
MPILIBNAME="$MPILIBNAME"
PMPILIBNAME="$PMPILIBNAME"
CC="$CC"
CPPFLAGS="$CPPFLAGS"
CFLAGS="$CFLAGS"
export LIBDIR ; export MPILIBNAME ; export CC ; export CPPFLAGS
export PMPILIBNAME
export CFLAGS
# For test/mpi/configure
MPI_SRCDIR=$MPI_SRCDIR
MPI_NO_SPAWN=$MPID_NO_SPAWN
export MPI_SRCDIR
export MPI_NO_SPAWN
])

if test -z "$includebuild_dir" ; then
    includebuild_dir=`pwd`/src/include
fi
if test -z "$libbuild_dir" ; then
    libbuild_dir=`pwd`/lib
fi
if test -z "$modincbuild_dir" ; then
    # The include files may be in a different directory from the mpif.h file
    modincbuild_dir=`pwd`/src/binding/fortran/use_mpi
fi

dnl
dnl If we rerun configure, place a file in the lib directory with the
dnl date.  We can use this to avoid rebuilding the library when
dnl a build aborts due to an error (this is intended to help developers)
AC_OUTPUT_COMMANDS([if [ ! -d lib ] ; then mkdir lib ; fi
date > lib/newconfig])

AC_OUTPUT_COMMANDS([chmod a+x test/commands/cmdtests])


AC_DEFINE(HAVE_MPICHCONF,1,[Define so that we can test whether the mpichconf.h file has been included])

if test "$USE_PMI2_API" = "yes" ; then
   AC_DEFINE(USE_PMI2_API, 1, [Define if PMI2 API must be used])
fi

########################################################################

# cause libtool script to be built now so that we can use it to test one last
# linking issue on Darwin
LT_OUTPUT

if test "X$enable_shared" = "Xyes" ; then
    # see ticket #1590 for some more background on these Darwin linking issues
    AS_CASE([$host],
        [*-*-darwin*],
        [

        # The linker on Darwin does not allow common symbols, thus libtool
        # adds the -fno-common option by default for shared libraries.
        # However, the common symbols defined in different shared libraries
        # and object files still can not be treated as the same symbol.
        # For example:
        # with gfortran, the same common block in the shared libraries and
        # the object files will have different memory locations separately;
        # with ifort, the same common block in different shared libraries
        # will get the same memory location but still get a different location
        # in the object file.

        # The -Wl,-commons,use_dylibs option asks linker to check dylibs for
        # definitions and use them to replace tentative definitions(commons)
        # from object files, thus it solves the issue of the common symbol
        # mismatch between the object file and the dylibs (i.e., by setting
        # the address of a common symbol to the place located in the first
        # dylib that is linked with the object file and contains this symbol).
        # It needs to be added only in the linking stage for the final
        # executable file.

        # On the other hand, the -flat-namespace option allows linker to
        # unify the same common symbols in different dylibs. It needs to be
        # added in linking stage for both the shared library and the final
        # executable file.

        # In Fortran programs, we implement global definitions such as MPI_BOTTOM
        # as common symbols. We check the address of the user input and translate it
        # to the C global definition if it is equal to the internal pre-stored address
        # in the Fortran binding layer. Without -flat-namespace, we may get different
        # addresses and thus treat the predefined constant as a normal buffer.

        # Although gfortran works fine by only adding -flat-namespace, and
        # ifort works by only adding -Wl,-commons,use_dylibs, we should add
        # both options here as a generic solution to make sure everything safe.

        # sanity check that -Wl,-flat_namespace works on darwin, unless the user
        # asked us not to add it
        if test "X$enable_two_level_namespace" = "Xno"; then
            # TODO, move this into a PAC macro with real autoconf caching
            pac_cv_wl_flat_namespace_works=no
            AC_MSG_CHECKING([if the C compiler accepts -Wl,-flat_namespace])
            PAC_PUSH_FLAG([LDFLAGS])
            PAC_APPEND_FLAG([-Wl,-flat_namespace],[LDFLAGS])
            AC_LINK_IFELSE([AC_LANG_PROGRAM([],[int i;])],
                           [AC_MSG_RESULT([yes])
                            pac_cv_wl_flat_namespace_works=yes],
                           [AC_MSG_RESULT([no])])
            PAC_POP_FLAG([LDFLAGS])

            # Technically we may not be able to use the same form of the argument
            # for all four compilers (CC/CXX/F77/FC).  But we only think this is
            # necessary for Darwin for now, so this unconditional, single-var
            # approximation will work for now.
            if test "X$pac_cv_wl_flat_namespace_works" = "Xyes" ; then
                PAC_APPEND_FLAG([-Wl,-flat_namespace], [LDFLAGS])
                PAC_APPEND_FLAG([-Wl,-flat_namespace], [WRAPPER_LDFLAGS])
            fi
        fi

        # We only need to bother with -Wl,-commons,-use_dylibs if we are
        # building fortran bindings (no common block usage in our C libs).
        if test "X$enable_f77" = "Xyes" ; then
            # TODO, move this into a PAC macro with real autoconf caching
            pac_cv_wl_commons_use_dylibs_works=no
            AC_MSG_CHECKING([if the F77 compiler accepts -Wl,-commons,use_dylibs])
            AC_LANG_PUSH([Fortran 77])
            PAC_PUSH_FLAG([LDFLAGS])
            PAC_APPEND_FLAG([-Wl,-commons,use_dylibs], [LDFLAGS])
            AC_LINK_IFELSE([AC_LANG_PROGRAM([],[      INTEGER i])],
                           [AC_MSG_RESULT([yes])
                            pac_cv_wl_commons_use_dylibs_works=yes],
                           [AC_MSG_RESULT([no])])
            PAC_POP_FLAG([LDFLAGS])
            AC_LANG_POP([Fortran 77])

            # Add the flag to the WRAPPER_LDFLAGS, since this common block issue
            # is really only a problem for dynamically linked user programs.
            #
            # Technically we may not be able to use the same form of the argument
            # for all four compilers (CC/CXX/F77/FC).  But we only think this is
            # necessary for Darwin for now, so this unconditional, single-var
            # approximation will work for now.
            if test "X$pac_cv_wl_commons_use_dylibs_works" = "Xyes" ; then
                PAC_APPEND_FLAG([-Wl,-commons,use_dylibs], [WRAPPER_LDFLAGS])
            fi
        fi
        ]
    )
fi

########################################################################
# Some of the settings need to be applied at the end
if test x"$pac_cc_strict_werror" = xyes ; then
    PAC_APPEND_FLAG([-Werror],[CFLAGS])
fi
########################################################################

if test -z "$pkgconfigdir" ; then
  # The default pkgconfig dir is under the lib dir
  pkgconfigdir=$libdir/pkgconfig
fi
AC_SUBST(pkgconfigdir)
export pkgconfigdir

dnl This includes an experimental pkgconfig file for ch3 in the src/pkgconfig
dnl directory
AC_OUTPUT(Makefile \
          examples/Makefile \
          test/Makefile \
          test/commands/Makefile \
          src/include/mpichinfo.h \
	  mpich-doxygen \
          src/include/mpir_ext.h \
          src/binding/cxx/mpicxx.h \
	  src/binding/fortran/mpif_h/mpif.h \
	  src/binding/fortran/mpif_h/setbotf.f \
	  src/binding/fortran/mpif_h/setbot.c \
	  src/binding/fortran/use_mpi/mpi_sizeofs.f90 \
	  src/binding/fortran/use_mpi/mpi_base.f90 \
	  src/binding/fortran/use_mpi/mpi_constants.f90 \
	  src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90 \
	  src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90 \
	  src/packaging/pkgconfig/mpich.pc \
	  src/packaging/envmods/mpich.module \
          src/env/mpixxx_opts.conf \
          src/env/mpicc.sh \
	  src/env/mpicc.bash \
          src/env/mpicxx.sh \
	  src/env/mpicxx.bash \
	  src/env/mpif77.sh \
	  src/env/mpif77.bash \
	  src/env/mpifort.sh \
	  src/env/mpifort.bash \
          src/env/parkill \
          src/include/mpi.h \
          doc/design/Makefile \
          doc/installguide/Makefile \
          doc/logging/Makefile \
          doc/refman/Makefile \
          doc/userguide/Makefile \
          test/commands/cmdtests)

echo 'Configuration completed.'
