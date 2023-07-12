##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

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
dnl    MPID_NO_LONG_LONG     - if yes, the device does not support the
dnl                            long long integer type
dnl    MPID_NO_LONG_DOUBLE   - if yes, the device does not support the
dnl                            long double type
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
dnl To let other scripts, e.g. configure in hydra and romio, 
dnl know that they are being invoked from within the MPICH configure,
dnl the following environment variables are set and exported:
dnl    FROM_MPICH
dnl
dnl Note that no executable statements are allowed (and any are silently
dnl dropped) before AC_INIT.

m4_define([MPICH_CONFIGURE], 1)
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
            AS_HELP_STRING([--with-custom-version-string], [Adds a user-specified value to the output of the mpichversion executable]),,with_custom_version_string="")
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
AC_CONFIG_HEADERS([src/include/mpichconf.h])
AH_TOP([/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

# confdb routines are used in multiple components. Let the macros know
# whether WRAPPER FLAGS are used.
m4_define([use_wrapper_flags], [1])

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

# Save a copy of precious flags as USER_* before any of these flags
# are being modified by configure tests.
PAC_PREFIX_ALL_FLAGS(USER)

dnl include all subsystem m4 fragments now that the core autoconf functionality
dnl has been setup.  No fragment should do anything except define
dnl PAC_SUBCFG_{PREREQ,BODY} macros which will be expanded later as
dnl appropriate
# begin subsys includes
m4_include([subsys_include.m4])
# end subsys includes

dnl ----------------------------------------------------------------------------
dnl setup top-level argument handling

AC_ARG_WITH(configfile,
        AS_HELP_STRING([--with-configfile], [Set global config file. The default is /etc/mpich.conf.])
,,with_configfile="/etc/mpich.conf")
if test "$with_configfile" != "no" ; then
    case "$with_configfile" in
        "yes|default")
            configfile="/etc/mpich.conf"
            ;;
        *)
            configfile="$with_configfile"
            ;;
    esac
    AC_DEFINE_UNQUOTED(USE_CONFIGFILE, ["$configfile"], [define to use global config file])
fi

AC_ARG_ENABLE(echo,
	AS_HELP_STRING([--enable-echo], [Turn on strong echoing. The default is enable=no.]),
	set -x)

AC_ARG_ENABLE(error-checking,
[  --enable-error-checking=level
      Control the amount of error checking.
        no        - no error checking
        runtime   - error checking controllable at runtime through environment
                    variables
        all       - error checking always enabled (default)
],,enable_error_checking=runtime)

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

AC_ARG_ENABLE(extended-context-bits,
[  --enable-extended-context-bits=yes|no - Enable wider context id space. (experimental)
        yes       - 20 bits
        no        - 16 bits (default)
],,enable_extended_context_bits=no)

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
        asan     - Add compiler flag, -fsanitize=address, to all internal
                   compiler flags. It also includes other flags for nicer
                   stacktrace such as -g -O1 -fno-omit-frame-pointer.
        ubsan    - Add compiler flag, -fsanitize=undefined, to all internal
                   compiler flags.
        mem      - Memory usage tracing
        meminit  - Preinitialize memory associated structures and unions to
                   eliminate access warnings from programs like valgrind
        memarena - Check for overwrite errors in memory allocation arena
        mutex    - Enable error checking on pthread mutexes
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
        multinic - All multinic-related
        all      - All variables above
],[],[enable_mpit_pvars=none])

AC_ARG_ENABLE([mpit-events],
[ --enable-mpit-events=yes|no - Enable MPI_T events. (Default is "no"):
        yes    - All available events
        no     - No events
],[],[enable_mpit_events=no])

dnl We may want to force MPI_Aint to be the same size as MPI_Offset,
dnl particularly on 32 bit systems with large (64 bit) file systems.
AC_ARG_WITH(aint-size,
	AS_HELP_STRING([--with-aint-size], [Override the size of MPI_AINT (in bytes)]),,
	with_aint_size=0)

AC_ARG_ENABLE(fast,
[  --enable-fast=option - Control the level of fast execution in the
                         MPICH implementation.  option is a list of
                         comma separated names including
        O<n>     - Appends default optimization flags, -O<n>, to all internal
                   compiler flags, i.e. MPICHLIB_CFLAGS, MPICHLIB_CXXFLAGS,
                   MPICHLIB_FFLAGS, and MPICHLIB_FCFLAGS. (default is -O2)
        ndebug   - Appends -DNDEBUG to MPICHLIB_CFLAGS.
        no-strict-alignment - relax strict alignment requirement
        alwaysinline - Force compiler to always inline performance critical routines
        all|yes  - "O2", "ndebug", and "alwaysinline" are enabled
        none     - None of above options, i.e. --disable-fast
],,enable_fast=O2)

AC_ARG_ENABLE(interlib-deps,
	[AS_HELP_STRING([--enable-interlib-deps - Enable interlibrary dependencies])],,enable_interlib_deps=yes)

AC_ARG_ENABLE(check-compiler-flags,
	AS_HELP_STRING([--enable-check-compiler-flags], [enable the checks for all compiler
                       options, xxxFLAGS, MPICH_xxxFLAGS. Default is on.]),,
		       enable_check_compiler_flags=yes)

dnl Whether to disable the Fortran bindings. By default, we'll probe and build all supported
dnl interfaces including mpif.h, use mpi, and use_mpi_f08.
AC_ARG_ENABLE(fortran,
        AS_HELP_STRING([--disable-fortran], [Whether to disable Fortran bindings]),,[enable_fortran=yes])

AC_ARG_ENABLE(f77,
        AS_HELP_STRING([--disable-f77], [Whether to disable "include 'mpif.h'" interface]),,[enable_f77=yes])

AC_ARG_ENABLE(f90,
        AS_HELP_STRING([--disable-f90], [Whether to disable "use mpi" interface]),,[enable_f90=yes])

AC_ARG_ENABLE(f08,
        AS_HELP_STRING([--disable-f08], [Whether to disable "use mpi_f08" interface]),,[enable_f08=yes])

AC_ARG_ENABLE(cxx,
	AS_HELP_STRING([--enable-cxx], [Enable C++ bindings]),,enable_cxx=yes)

AC_ARG_ENABLE(romio,
	AS_HELP_STRING([--enable-romio], [Enable ROMIO MPI I/O implementation]),,
	enable_romio=yes)

AC_ARG_ENABLE(debuginfo,
	AS_HELP_STRING([--enable-debuginfo], [Enable support for debuggers]),,
	enable_debuginfo=no)


## Enable creation of libtool-style versioning or no versioning
AC_ARG_ENABLE(versioning,
        [AS_HELP_STRING([--enable-versioning],[Enable library versioning])],,
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
	AS_HELP_STRING([--with-device=name], [Specify the communication device for MPICH]),,
	with_device=ch4)

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
	AS_HELP_STRING([--enable-thread-cs=type],
			[Choose the method used for critical sections
                         and other atomic updates when multiple
                         threads are present.  Values may be default, global,
                         per-vci, lock-free. Default is global for ch3 and per-vci for ch4.
                        ]),,enable_thread_cs=default)

AC_ARG_ENABLE(refcount,
	AS_HELP_STRING([--enable-refcount=type],
			[Choose the method for ensuring atomic updates
                         to the reference counts for MPI objects.
                         Values may be lock-free or none.  The
                         default depends on the thread-cs choice; for
                         global it is none (because none is required),
                         for per-vci, and lock-free, lock-free]),,
                         enable_refcount=default)

AC_ARG_ENABLE(mutex-timing,
	AS_HELP_STRING([--enable-mutex-timing], [calculate the time spent waiting on mutexes]),
	AC_DEFINE(MPIU_MUTEX_WAIT_TIME,1,[Define to enable timing mutexes]))

AC_ARG_ENABLE([predefined-refcount],
	AS_HELP_STRING([--enable-predefined-refcount],
                       [control whether predefined objects like
		       MPI_COMM_WORLD are reference counted (default
		       depends on --enable-thread-cs choice)]),[],
              [enable_predefined_refcount=default])

AC_ARG_ENABLE(weak-symbols,
	AS_HELP_STRING([--enable-weak-symbols],
			[Use weak symbols to implement PMPI routines (default)]),,
		enable_weak_symbols=yes)

AC_ARG_ENABLE(qmpi,
	AS_HELP_STRING([--enable-qmpi],
			[Enable QMPI support (default)]),,
		enable_qmpi=no)

if test "$enable_qmpi" = "yes" ; then
    AC_DEFINE(ENABLE_QMPI,1,[Define if QMPI enabled])
    ENABLE_QMPI=1
else
    ENABLE_QMPI=0
fi
AC_SUBST(ENABLE_QMPI)

AC_ARG_ENABLE([two-level-namespace],
              [AS_HELP_STRING([--disable-two-level-namespace],
                              [(Darwin only) Add `-Wl,-flat_namespace` to
                               mpicc/mpifort/etc. compiler wrappers.
                               This may fix some issues due to not resolving
                               MPI constants, such as MPI_IN_PLACE.
                               Try disable this option if you encounter these issues
                               on Mac OS.])],
              [],
              [enable_two_level_namespace=yes])

AC_ARG_ENABLE(multi-aliases,
	AS_HELP_STRING([--enable-multi-aliases],
		[Multiple aliasing to support multiple fortran compilers (default)]),,
		enable_multi_aliases=yes)

AC_ARG_WITH([wrapper-dl-type],
            [AS_HELP_STRING([--enable-wrapper-dl-type],
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
              [AS_HELP_STRING([--disable-long-double],
                              [Pass --disable-long-double to prevent the MPI
                               library from supporting the C "long double" type,
                               even if the C compiler supports it.  "long
                               double" support is enabled by default, provided
                               the compiler supports it.])],
              [],
              [enable_long_double=yes])

AC_ARG_WITH(cross,
	AS_HELP_STRING([--with-cross=file],
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

AC_ARG_ENABLE(nolocal, AS_HELP_STRING([--enable-nolocal], [enables nolocal mode where shared-memory communication is disabled]),
    AC_DEFINE(ENABLE_NO_LOCAL, 1, [Define to disable shared-memory communication]))

# options passed to sub configures
AC_ARG_WITH([cuda], [AS_HELP_STRING([--with-cuda=[[PATH]]],PAC_WITH_LIB_HELP_STRING(cuda))])
AC_ARG_WITH([hip], [AS_HELP_STRING([--with-hip=[[PATH]]],PAC_WITH_LIB_HELP_STRING(hip))])
AC_ARG_WITH([ze], [AS_HELP_STRING([--with-ze=[[PATH]]],PAC_WITH_LIB_HELP_STRING(ze))])

AC_CANONICAL_TARGET

# Find a C compiler.
# We also need to do this before the F77 and FC test to ensure that we
# find the C preprocessor reliably.
AC_PROG_CC
AC_PROG_CC_C99
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

AC_USE_SYSTEM_EXTENSIONS
PAC_C_NO_COMMON

dnl now that autoconf and core compilers are setup, init automake and libtool
dnl
dnl We would like to pass -Werror, but we are cheating in the "examples/"
dnl directory and overriding the user-flags like CFLAGS, which automake-1.12
dnl warns about.  Long-term we may need to use a hand-written Makefile.in or
dnl something else in this special dir.
AM_INIT_AUTOMAKE([-Wall -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects no-dist])
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
#
# NOTE: we are skipping overriding CXX as some modules (e.g. ucx) may depend on CXX
#

# suppress default "-g -O2" from AC_PROG_CXX
: ${CXXFLAGS=""}
AC_PROG_CXX
AC_CACHE_CHECK([whether the C++ compiler $CXX can build an executable],
    pac_cv_cxx_builds_exe,[
    AC_LANG_PUSH([C++])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
        class mytest {
            int a;
        public:
            mytest(void) : a(1) {}
            ~mytest(void) {}
        };
        ]],[[mytest a;]])],
        pac_cv_cxx_builds_exe=yes, pac_cv_cxx_builds_exe=no)
    AC_LANG_POP([C++])
])
if test "$pac_cv_cxx_builds_exe" = "no" ; then
    # clear $CXX if C++ compiler does not work
    CXX=
fi

if test "$enable_fortran" = "no" ; then
    enable_f77=no
    enable_f90=no
    enable_f08=no
fi

# Python 3 is needed to generate Fortran bindings
PAC_CHECK_PYTHON

if test "$enable_fortran" != "no" ; then
    # suppress default "-g -O2" from AC_PROG_FC
    : ${FCFLAGS=""}
    AC_PROG_FC

    F77=$FC

    FFLAGS=$FCFLAGS
    AC_PROG_F77

fi

AM_CONDITIONAL([INSTALL_MPIF77],[false])

# compute canonical system types
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
# TARGET not needed, MPICH isn't a compiler

# Enable better caching control
PAC_ARG_CACHING

# Set CFLAGS for enable strict if necessary.  Do this *first* because
# it may influence the output of the other tests
PAC_ARG_STRICT

# To link C objects with Fortran main program may require -fPIE option
PAC_C_CHECK_fPIE_OK

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
if test -z "$main_top_srcdir" ; then
    # This needs to be an absolute pathname
    case "$use_top_srcdir" in
    /*) ;;
    *)
        use_top_srcdir=`(cd $use_top_srcdir && pwd)`
	;;
    esac
    main_top_srcdir=$use_top_srcdir
fi
# Get the directory that we're running in...
if test -z "$main_top_builddir" ; then
   main_top_builddir="`pwd`"
fi
AC_SUBST(main_top_builddir)
AC_SUBST(main_top_srcdir)
export main_top_builddir
export main_top_srcdir

if test -z "$with_cross"; then
  if test -f "$main_top_srcdir/src/cross/$host_alias"; then
    with_cross="$main_top_srcdir/src/cross/$host_alias"
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

# ----------------------------------------------------------------------------
# with-device
#
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

if test "$device_name" = "ch4" ; then
    AC_DEFINE([ENABLE_LOCAL_SESSION_INIT], 1, [Define to skip initializing builtin world comm during MPI_Session_init])
fi

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
        no-strict-alignment)
        enable_fast_no_strict_alignment=yes
        ;;
        alwaysinline) # No op in MPICH. See mpl/configure.ac
        ;;
        avx)
        enable_fast_avx_instr=yes
        ;;
        all|yes)
        enable_fast_ndebug=yes
        enable_fast_opts=O2
        ;;
        none|no)
        enable_fast_ndebug=no
        enable_fast_opts=O0
        enable_fast_avx_instr=no
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

if test -z "$enable_fast_no_strict_alignment" ; then
    # we need observe strict alignment to pass ubsan check
    AC_DEFINE(NEEDS_STRICT_ALIGNMENT,1,[Define if strict alignment memory access is required])
fi

if test "$enable_fast_avx_instr" = "yes" ; then
    AC_CACHE_CHECK([whether -mavx is supported], pac_cv_found_avx,
                   [PAC_C_CHECK_COMPILER_OPTION([-mavx],pac_cv_found_avx=yes,pac_cv_found_avx=no)],
                   pac_cv_found_avx=no,pac_cv_found_avx=yes)
    if test "$pac_cv_found_avx" = "yes" ; then
        PAC_APPEND_FLAG([-mavx],[CFLAGS])

        AC_CACHE_CHECK([whether _mm256_stream_si256 is supported], pac_cv_found__mm256_stream_si256,[
                        AC_RUN_IFELSE([AC_LANG_SOURCE([[
                                       #include <immintrin.h>

                                       int main(int argc, char **argv) {
                                           char source[1024], dest[1024];
                                           for (int i = 0; i < 1024; i++) source[i] = 'a';

                                           __m256i ymm0 = _mm256_loadu_si256((__m256i const *) source);
                                           _mm256_stream_si256((__m256i *) dest, ymm0);

                                           if (dest[0] == source[0]) return 0;
                                           else return 1;
                                       }
                                       ]])], pac_cv_found__mm256_stream_si256="yes",
                                       pac_cv_found__mm256_stream_si256="no",
                                       pac_cv_found__mm256_stream_si256="unknown")
                        ])
    fi
    if test "$pac_cv_found__mm256_stream_si256" = "yes" ; then
        AC_DEFINE(HAVE_MM256_STREAM_SI256,1,[Define if 256 bit streaming memcpy is available])
    fi
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

# context bits
if test "$enable_extended_context_bits" = "yes" ; then
    AC_DEFINE([HAVE_EXTENDED_CONTEXT_BITS],[1],[Define to enable extended context id bit space])
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

# MPL
mplsrcdir=""
AC_SUBST([mplsrcdir])
mpllibdir=""
AC_SUBST([mpllibdir])
mpllib=""
AC_SUBST([mpllib])

m4_define([mpl_embedded_dir],[src/mpl])
PAC_CONFIG_MPL

# json-c

jsonsrcdir=""
AC_SUBST([jsonsrcdir])
jsonlib=""
AC_SUBST([jsonlib])

jsonlib="modules/json-c/libjson-c.la"
if test -e "${use_top_srcdir}/modules/PREBUILT" -a -e "$jsonlib"; then
    jsonsrcdir=""
else
    PAC_PUSH_ALL_FLAGS()
    PAC_RESET_ALL_FLAGS()
    PAC_CONFIG_SUBDIR_ARGS([modules/json-c],[--enable-embedded --disable-werror],[],[AC_MSG_ERROR(json-c configure failed)])
    PAC_POP_ALL_FLAGS()
    jsonsrcdir="${main_top_builddir}/modules/json-c"
fi
PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/json-c],[CPPFLAGS])
PAC_APPEND_FLAG([-I${main_top_builddir}/modules/json-c],[CPPFLAGS])

# ----------------------------------------------------------------------------
# HWLOC / NETLOC
# ----------------------------------------------------------------------------
hwlocsrcdir=""
AC_SUBST([hwlocsrcdir])
hwloclib=""
AC_SUBST([hwloclib])

m4_define([hwloc_embedded_dir],[modules/hwloc])
PAC_CONFIG_HWLOC

if test "$pac_have_hwloc" = "yes" ; then
    AC_DEFINE(HAVE_HWLOC,1,[Define if hwloc is available])
fi

PAC_CHECK_HEADER_LIB_OPTIONAL([netloc],[netloc.h],[netloc],[netloc_get_all_host_nodes])
if test "$pac_have_netloc" = "yes" ; then
    AC_DEFINE(HAVE_NETLOC,1,[Define if netloc is available])
fi

# ----------------------------------------------------------------------------
# Threads
# ----------------------------------------------------------------------------
# config time:
# * MPID_MAX_THREAD_LEVEL, set in one of the device subconfigure.m4,
#    determines the maximum thread level, typically MPI_THREAD_MULTIPLE.
#    If missing, equivallent to single.
# * --enable-threads, explicit user config option, raise error if beyond
#    MPID_MAX_THREAD_LEVEL.
#    It default to MPID_MAX_THREAD_LEVEL.
# * MPICH_THREAD_LEVEL is set by enable_threads
# * MPICH_IS_THREADED is set when MPICH_THREAD_LEVEL==MPI_THREAD_MULTIPLE
# runtime:
# * MPI_Init checks MPIR_CVAR_DEFAULT_THREAD_LEVEL, default to single
# * MPI_Init_thread may request thread level explicitly
# * MPID_Init_thread_level may overwrite the requested thread level
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
    # default depends on ch3 or ch4
    if test "$enable_thread_cs" = "default" ; then
        if test "$device_name" = "ch4" ; then
            enable_thread_cs="per-vci"
        else
            enable_thread_cs="global"
        fi
    fi

    case $enable_thread_cs in
    global)
    thread_granularity=MPICH_THREAD_GRANULARITY__GLOBAL
    if test "$enable_refcount" = "default" ; then enable_refcount=none ; fi
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

if test "$thread_granularity" = "MPICH_THREAD_GRANULARITY__VCI" -a "$device_name" != "ch4" ; then
    AC_MSG_ERROR([per-vci critical section granularity is only supported for the ch4 device])
fi

if test "$enable_predefined_refcount" = "no" ; then
    AC_DEFINE([MPICH_THREAD_SUPPRESS_PREDEFINED_REFCOUNTS],[1],[define to disable reference counting predefined objects like MPI_COMM_WORLD])
fi

AC_DEFINE_UNQUOTED([MPICH_THREAD_REFCOUNT],$thread_refcount,[Method used to implement refcount updates])

# threadcomm feature depends on compiler tls (checked by MPL) and pervci CS
if test -n "$COMPILER_TLS" -a "$thread_granularity" = "MPICH_THREAD_GRANULARITY__VCI" ; then
    AC_DEFINE([ENABLE_THREADCOMM], 1, "set to enable threadcomm feature")
fi

# enable-g
# strip off multiple options, separated by commas
save_IFS="$IFS"
IFS=","
for option in $enable_g ; do
    case "$option" in
        debug|dbg)
        enable_append_g=yes
	;;
        asan)
        enable_asan=yes
        ;;
        ubsan)
        enable_ubsan=yes
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
        most|yes)
        perform_memtracing=yes
        enable_append_g=yes
        perform_meminit=yes
        perform_dbgmutex=yes
        perform_handlealloc=yes
        perform_handle=yes
        ;;
	all)
        perform_memarena=yes
	perform_memtracing=yes
	enable_append_g=yes
	perform_meminit=yes
	perform_dbgmutex=yes
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

if test "$enable_asan" = "yes" ; then
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[CFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[CXXFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[FFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[FCFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[WRAPPER_CFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[WRAPPER_CXXFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[WRAPPER_FFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[WRAPPER_FCFLAGS])
    PAC_APPEND_FLAG([-fsanitize=address],[LDFLAGS])
fi
if test "$enable_ubsan" = "yes" ; then
    PAC_APPEND_FLAG([-fsanitize=undefined],[CFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[CXXFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[FFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[FCFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[WRAPPER_CFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[WRAPPER_CXXFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[WRAPPER_FFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[WRAPPER_FCFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[LDFLAGS])
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

pac_cross_compiling=no
if test "$with_cross" != "no" ; then
    if test -s "$with_cross" ; then
        AC_MSG_RESULT([Reading values from cross-compilation file $with_cross])
        . $with_cross
	# Autoconf 2.52 no longer sets cross_compiling except with the
	# awkward "targethost" options.
	pac_cross_compiling=yes
	cross_compiling=yes
	ac_cv_prog_cc_cross=yes
	ac_cv_prog_f77_cross=yes
 	ac_cv_prog_fc_cross=yes
        ac_cv_prog_cxx_cross=yes
        export cross_compiling
	# Export all cross variables.  Any subsidiary configure should also
        # export CROSS_xxx
	rm -f confcross
	(set) 2>&1 | grep CROSS_ | \
	      sed -e 's/^/export /g' -e 's/=.*//g' > confcross
	. ./confcross
	rm -f confcross
    fi
fi

# This goes here because we need the top_srcdir
if test "$enable_romio" = "yes" ; then
   if test -d $use_top_srcdir/src/mpi/romio ; then
       AC_DEFINE(HAVE_ROMIO,1,[Define if ROMIO is enabled])

       # make it possible to "#include" mpio.h at build time
       #
       # This ought to be sufficient, but there is also a symlink setup in
       # src/include to accommodate current mpicc limitations.  See
       # src/mpi/Makefile.mk for more info.
       PAC_APPEND_FLAG([-I${main_top_builddir}/src/mpi/romio/include],[CPPFLAGS])

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
       PAC_CONFIG_SUBDIR_ARGS([src/mpi/romio],[],[],[AC_MSG_ERROR([src/mpi/romio configure failed])])
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

# ---------------------------------------------------------------------------
# Configuring pmi, pmilib, and pm
#

AC_ARG_WITH(pmi, [
  --with-pmi=name - Specify the pmi interface for MPICH.
                      pmi1 - use PMIv1 (default)
                      pmi2 - use PMI2
                      pmix - use PMIx
],, with_pmi=pmi1)

AC_ARG_WITH(pmilib, [
  --with-pmilib=option -  [Deprecated] Specify the PMI library. Use --with-pmi=[PATH],
                          or --with-pmi2=[PATH], or --with-pmix=[PATH] to specify
                          PMI library path.
],,with_pmilib=mpich)

AC_ARG_WITH(pm,
	AS_HELP_STRING([--with-pm=name],
		[Specify the process manager for MPICH.  "no" or "none" are
                 valid values.  Multiple process managers may be specified as
                 long as they all use the same pmi interface by separating them
                 with colons.  The mpiexec for the first named process manager
                 will be installed.  Example: "--with-pm=hydra:gforker"
                 builds the two process managers hydra, and gforker;
                 only the mpiexec from hydra is installed into the bin
                 directory.]),,with_pm=default)

# --- check and normalize the options ----

case "$with_pmi" in
  pmi1|pmi2|pmix)
    ;;
  slurm)
    # deprecate
    with_pmi=pmi1
    with_pmilib=slurm
    with_pm=no
    ;;
  oldcray)
    # deprecate
    with_pmi=pmi1
    with_pmilib=oldcray
    with_pm=no
    ;;
  *)
    # allow user to specify an external libpmi
    PAC_CHECK_HEADER_LIB_FATAL([pmi], [pmi.h], [pmi], [PMI_Init])
    # assume using PMI-1
    with_pmi=pmi1
    with_pmilib=no
    ;;
esac

PAC_CHECK_HEADER_LIB_EXPLICIT(pmi2, pmi2.h, pmi2, PMI2_Init)
if test "$pac_have_pmi2" = "yes" ; then
    with_pmi=pmi2
    with_pmilib=no
fi

PAC_CHECK_HEADER_LIB_EXPLICIT(pmix, pmix.h, pmix, PMIx_Init)
if test "$pac_have_pmix" = "yes" ; then
    with_pmi=pmix
    with_pmilib=no
    with_pm=no
fi

# --- $with_pmi ----

case "$with_pmi" in
  pmi2)  
    AC_DEFINE(USE_PMI2_API, 1, [Define if PMI2 API must be used])
    ;;
  pmix)  
    AC_DEFINE(USE_PMIX_API, 1, [Define if PMIx API must be used])
    ;;
esac

# --- $with_pm ----

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

for pm_name in $pm_names ; do
    if test -z "$first_pm_name" ; then
       first_pm_name=$pm_name
       export first_pm_name
    fi

    case "$pm_name" in
      gforker)
        build_gforker=yes
        if test "$with_pmi" != "pmi1" ; then
            AC_MSG_ERROR([$pm_name requires PMI-v1, but $with_pmi was chosen.])
        fi
        ;;
      remshell)
        build_refshell=yes
        if test "$with_pmi" != "pmi1" ; then
            AC_MSG_ERROR([$pm_name requires PMI-v1, but $with_pmi was chosen.])
        fi
        ;;
      hydra)
        if test "$with_pmi" = "pmix" ; then
            AC_MSG_ERROR([$pm_name does not support $with_pmi.])
        fi
        subsystems="$subsystems src/pm/hydra"
        ;;
      *)
        AC_MSG_WARN([PM $pm_name is unknown])
	hasError=yes
        ;;
    esac
done

if test "$hasError" != no ; then
    AC_MSG_ERROR([Aborting configure because an error was seen in the selection of process managers])
fi
#
# pm_name is the *primary* pm
pm_name=$first_pm_name
AC_SUBST(pm_name)

dnl
dnl gforker and remshell does not use separate configure. 
dnl
AM_CONDITIONAL([BUILD_PM_GFORKER],[test "X$build_gforker" = "Xyes"])
AM_CONDITIONAL([BUILD_PM_REMSHELL],[test "X$build_refshell" = "Xyes"])
AM_CONDITIONAL([PRIMARY_PM_GFORKER],[test "X$first_pm_name" = "Xgforker"])
AM_CONDITIONAL([PRIMARY_PM_REMSHELL],[test "X$first_pm_name" = "Xremshell"])

# ---- $with_pmilib ----

pmisrcdir=""
AC_SUBST([pmisrcdir])
pmilib=""
AC_SUBST([pmilib])

case "$with_pmilib" in
  mpich|install)
    if test "$with_pmi" != "pmi1" -a "$with_pmi" != "pmi2" ; then
        AC_MSG_ERROR([pmilib=$with_pmilib is incompatible with $with_pmi]);
    fi
    pmisrcdir="src/pmi"
    pmilib="src/pmi/libpmi.la"

    pmi_subdir_args=""
    if test "$with_pmilib" != "install" ; then
        pmi_subdir_args="--enable-embedded"
    fi
    PAC_CONFIG_SUBDIR_ARGS([src/pmi], [$pmi_subdir_args])

    PAC_APPEND_FLAG([-I${use_top_srcdir}/src/pmi/include], [CPPFLAGS])
    PAC_APPEND_FLAG([-I${main_top_builddir}/src/pmi/include], [CPPFLAGS])
    ;;
  slurm)
    PAC_SET_HEADER_LIB_PATH([slurm])
    if test "$with_pmi" = "pmi1" ; then
        AC_CHECK_HEADER([slurm/pmi.h], [], [AC_MSG_ERROR([could not find slurm/pmi.h.  Configure aborted])])
        AC_CHECK_LIB([pmi], [PMI_Init],
                    [PAC_PREPEND_FLAG([-lpmi],[LIBS])
                     PAC_PREPEND_FLAG([-lpmi], [WRAPPER_LIBS])],
                    [AC_MSG_ERROR([could not find the Slurm libpmi.  Configure aborted])])
        AC_DEFINE([USE_PMI1_SLURM], 1, [Define if using Slurm PMI 1])
    elif test "$with_pmi" = "pmi2" ; then
        AC_CHECK_HEADER([slurm/pmi2.h], [], [AC_MSG_ERROR([could not find slurm/pmi2.h.  Configure aborted])])
        AC_CHECK_LIB([pmi2], [PMI2_Init],
                    [PAC_PREPEND_FLAG([-lpmi2],[LIBS])
                     PAC_PREPEND_FLAG([-lpmi2], [WRAPPER_LIBS])],
                    [AC_MSG_ERROR([could not find the Slurm libpmi2.  Configure aborted])])
        AC_DEFINE([USE_PMI2_SLURM], 1, [Define if using Slurm PMI 2])
    fi
    ;;
  oldcray)
    # deprecate
    PAC_PREPEND_FLAG([$CRAY_PMI_INCLUDE_OPTS], [CPPFLAGS])
    PAC_PREPEND_FLAG([$CRAY_PMI_POST_LINK_OPTS], [LDFLAGS])

    AC_CHECK_HEADER([pmi.h], [], [AC_MSG_ERROR([could not find pmi.h.  Configure aborted])])
    AC_CHECK_LIB([pmi], [PMI_Init], [craypmi_libpmi=yes], [craypmi_libpmi=no])

    if test "X$craypmi_libpmi" = "Xyes"; then
        PAC_APPEND_FLAG([-lpmi], [WRAPPER_LIBS])
    else
        AC_CHECK_LIB([pmi2], [PMI_Init], [craypmi_libpmi2=yes], [craypmi_libpmi2=no])
        if test "X$craypmi_libpmi2" = "Xyes"; then
            PAC_APPEND_FLAG([-lpmi2], [WRAPPER_LIBS])
        else
            AC_MSG_ERROR([could not find the cray libpmi.  Configure aborted])
        fi
    fi

    if test "$with_pmi" = "pmi2" ; then
        AC_DEFINE([USE_PMI2_CRAY], 1, [Define if using CRAY PMI 2])
    fi
    ;;
  bgq)
    # deprecate
    # This is a hack to include the pmi.h header. The OFI/BGQ provider
    # includes PMI functions, but no header file.
    PAC_PREPEND_FLAG([-I${use_top_srcdir}/src/pmi/include], [CPPFLAGS])
    AC_DEFINE([NO_PMI_SPAWN_MULTIPLE], 1, [The PMI library does not have PMI_Spawn_multiple.])
    ;;
  no)
    # taken care of by --with-pmi=path, or --with-pmi2=path, or --with-pmix=path
    ;;
  *)
    AC_MSG_ERROR([pmilib $with_pmilib not supported.])
    ;;
esac

# ---------------------------------------------------------------------------
# Check for whether the compiler defines a symbol that contains the
# function name. The MPICH code uses this for debugging purposes.

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
            [multinic],[enable_pvar_multinic=yes],
            [all|yes],
            [enable_pvar_nem=yes
             enable_pvar_recvq=yes
             enable_pvar_rma=yes
             enable_pvar_dims=yes
             enable_pvar_multinic=yes
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
# Support for MPI_T Events
if test "$enable_mpit_events" = "yes" ; then
    AC_DEFINE(HAVE_MPIT_EVENTS, [1], [Define if MPI_T Events are enabled])
fi

if test -n "$enable_pvar_multinic" ; then
    status_multinic_pvars=1
else
    status_multinic_pvars=0
fi
AC_DEFINE_UNQUOTED(ENABLE_PVAR_MULTINIC,$status_multinic_pvars,
          [Define to 1 to enable message count transmitted through multiple NICs MPI_T performance variables])

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
# Handle default choices for the Fortran compilers
# Note that these have already been set above

if test "$enable_f90" = "yes" -o "$enable_f08" = "yes"; then
    if test "$FC" = "" -o "$FC" = "no"; then
        # No Fortran compiler found; abort
        AC_MSG_ERROR([No Fortran compiler found. If you don't need to
        build any Fortran programs, you can disable Fortran support using
        --disable-fortran. If you do want to build Fortran
        programs, you need to install a Fortran compiler such as gfortran
        or ifort before you can proceed.])
    fi

    # Determine the extension for Fortran 90 files (it isn't always .f90)
    PAC_FC_EXT
    FCEXT=$ac_fc_srcext
    AC_SUBST(FCEXT)
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
       AS_HELP_STRING([--enable-f77characterlen],
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

if test "$enable_f90" = "yes" -a "$enable_f77" != "yes" ; then
   # Fortran 90 support requires compatible Fortran 77 support
   AC_MSG_ERROR([
Fortran 90 support requires compatible Fortran 77 support.
To force the use of the Fortran 90 compiler for Fortran 77,
do not use configure option --disable-fortran, and set the environment
variable F77 to the name of the Fortran 90 compiler, or \$FC.
If you do not want any Fortran support, use configure options
--disable-fortran.])
fi

# ----------------------------------------------------------------------------
# Now test for Fortran compiler characteristics
# ----------------------------------------------------------------------------
if test "$enable_f77" = "yes" ; then
    # determine shared library flags for F77
    f77_shlib_conf=src/env/f77_shlib.conf
    PAC_COMPILER_SHLIB_FLAGS([F77],[$f77_shlib_conf])
    AC_SUBST_FILE([f77_shlib_conf])

    AC_LANG([Fortran 77])
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
        # Code using mpif.h interface will likely need this flag to compile.
        # Code with `use mpi` or `use mpi_f08` do not need this flag.
        # Add the flag to mpif77 wrappers.
        WRAPPER_EXTRA_F77_FLAGS="$addarg"
    fi
    AC_SUBST(WRAPPER_EXTRA_F77_FLAGS)

    bindings="$bindings f77"
    AC_DEFINE(HAVE_FORTRAN_BINDING,1,[Define if Fortran is supported])
    # Also define the name FORTRAN_BINDING for use in #if @FORTRAN_BINDING@..
    FORTRAN_BINDING=1

fi

dnl By modifying mpif.h to use ! for comments, it can work with many f90
dnl compilers without creating a separate version.
dnl Note that this is run AFTER the AC_OUTPUT actions
AC_CONFIG_COMMANDS([src/include/mpif.h],[if test "$enable_f77" = yes ; then
if test "$has_exclaim" = "yes" ; then
    sed -e 's/^C/\!/g' src/binding/fortran/mpif_h/mpif.h > src/include/mpif.h
	cp src/include/mpif.h src/binding/fortran/mpif_h/mpif.h
else
    cp src/binding/fortran/mpif_h/mpif.h src/include
fi
if test "$has_fort_real8" = "yes" ; then
    sed -e 's/DOUBLE PRECISION/REAL*8/g' src/include/mpif.h > src/include/mpif.h.new
    mv src/include/mpif.h.new src/include/mpif.h
    cp src/include/mpif.h src/binding/fortran/mpif_h/mpif.h
fi
fi],
main_top_srcdir=$main_top_srcdir
enable_f77=$enable_f77
has_exclaim=$has_exclaim
has_fort_real8=$pac_cv_fort_real8
includebuild_dir=$includebuild_dir
libbuild_dir=$libbuild_dir
bashWorks=$bashWorks)

if test "$enable_f90" = "yes" ; then
    bindingsubsystems="$bindingsubsystems src/binding/fortran/use_mpi"
    bindings="$bindings f90"
fi

if test "$enable_f08" = "yes" ; then
    PAC_FC_2008_SUPPORT([:],[enable_f08=no])
fi
AM_CONDITIONAL([BUILD_F08_BINDING], [test "$enable_f08" = "yes"])

if test "$enable_f08" = "yes" ; then
    bindings="$bindings f08"
    AC_DEFINE(HAVE_F08_BINDING, 1, [Define to 1 if we have Fortran 2008 binding])
fi

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

if test "$enable_f90" = "yes" -o "$enable_f08" = "yes"; then
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
    if test -z "$CXX" ; then
        AC_MSG_ERROR([Aborting because C++ compiler does not work.  If you do not need a C++ compiler, configure with --disable-cxx])
    fi
    # Recent failures have come when a standard header is loaded
    # The Intel icpc 10.x compiler fails with <string> if gcc 4.2 is installed.
    AC_CACHE_CHECK([whether C++ compiler works with string],pac_cv_cxx_compiles_string,[
    AC_LANG_PUSH([C++])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string>]],[[return 0;]])],
        pac_cv_cxx_compiles_string=yes,pac_cv_cxx_compiles_string=no)
    AC_LANG_POP([C++])
])
    if test "$pac_cv_cxx_compiles_string" != yes ; then
        AC_MSG_WARN([The C++ compiler $CXX cannot compile a program containing the <string> header - this may indicate a problem with the C++ installation.  Consider configuing with --disable-cxx])
    fi

    AC_LANG([C++])
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
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <iostream>]],[[using namespace std;]])],
        pac_cv_cxx_has_iostream=yes,pac_cv_cxx_has_iostream=no)])
    AX_CXX_NAMESPACE_STD

    AC_CACHE_CHECK([whether <math> available],pac_cv_cxx_has_math,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <math>]],[[using namespace std;]])],
        pac_cv_cxx_has_math=yes,pac_cv_cxx_has_math=no)])

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
         AC_RUN_IFELSE([AC_LANG_SOURCE([[
            #include <stdio.h>
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
            }]])],ac_vals=`cat conftest.out`)
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
fi

# we install mpicxx as long as CXX is defined. In the case of enable_cxx=no,
# it is essentially the same as mpicc with CXX compiler.
if test -n "$CXX" ; then
    # determine shared library flags for CXX
    cxx_shlib_conf=src/env/cxx_shlib.conf
    PAC_COMPILER_SHLIB_FLAGS([CXX],[$cxx_shlib_conf])
    AC_SUBST_FILE([cxx_shlib_conf])

    if test "$enable_cxx" != yes; then
        MPICXXLIBNAME="$MPILIBNAME"
    fi
fi
AM_CONDITIONAL([INSTALL_MPICXX],[test -n "$CXX"])

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

AC_LANG([C])
#
# ----------------------------------------------------------------------------
# Done with the basic argument processing and decisions about which
# subsystems to build
# ----------------------------------------------------------------------------

# Look for perl.  Perl is needed in configure MPL (confdb/cmd_prefix_config_h.pl).
AC_PATH_PROG(PERL,perl,[no])
if test "$PERL" = "no" ; then
    AC_MSG_ERROR([Perl is required but not found.])
fi

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
   if test "$ac_cv_c_compiler_gnu" = "yes" ; then
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

# We need to check for the endianness in order to implement the
# "external32" representations.  This defines "WORDS_BIGENDIAN when
# the system is bigendian.
# As of autoconf 2.62, this macro takes an additional argument for systems
# that can produce object files for either endianness.
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
    AC_MSG_ERROR([Unable to determine endianness])
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
AC_CACHE_CHECK([whether $CC supports __func__],pac_cv_cc_has___func__, [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[[const char *cp = __func__;]])],
        pac_cv_cc_has___func__=yes, pac_cv_cc_has___func__=no)
])
if test "$pac_cv_cc_has___func__" != "yes" ; then
    AC_CACHE_CHECK([whether $CC supports __FUNCTION__],pac_cv_cc_has___FUNCTION__,[
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[[const char *cp = __FUNCTION__;]])],
            pac_cv_cc_has___FUNCTION__=yes, pac_cv_cc_has___FUNCTION__=no)
    ])
fi

# ----------------------------------------------------------------------------
# Check on support for long double and long long types.  Do this before the
# structure alignment test because it will test for including those
# types as well.  In addition, allow the device to suppress support for these
# optional C types by setting MPID_NO_LONG_DOUBLE and/or MPID_NO_LONG_LONG
# to yes.
if test "$MPID_NO_LONG_DOUBLE" != "yes" && test "X$enable_long_double" != "Xno" ; then
    AC_CACHE_CHECK([whether long double is supported],
    pac_cv_have_long_double,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[[long double a;]])],
        pac_cv_have_long_double=yes,pac_cv_have_long_double=no)])
    if test "$pac_cv_have_long_double" = "yes" ; then
        AC_DEFINE(HAVE_LONG_DOUBLE,1,[Define if long double is supported])
    fi
fi
if test "$MPID_NO_LONG_LONG" != "yes" ; then
    AC_CACHE_CHECK([whether long long is supported],
    pac_cv_have_long_long,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[[long long a;]])],
        pac_cv_have_long_long=yes,pac_cv_have_long_long=no)])
    if test "$pac_cv_have_long_long" = "yes" ; then
        AC_DEFINE(HAVE_LONG_LONG_INT,1,[Define if long long is supported])
    fi
fi
# Add HAVE_FLOAT16 detection
if test "$MPID_NO_FLOAT16" != "yes" ; then
    AC_CACHE_CHECK([whether _Float16 is supported],
    pac_cv_have_float16,[
    # The compiler might optimize out _Float16 usage and give a false
    # positive for this test (observed with clang 6 on Linux with -O2).
    # We use volatile as a way to disable optimizations.
    AC_LINK_IFELSE([AC_LANG_PROGRAM([],[volatile _Float16 a=1; return (int)a;])],
    pac_cv_have_float16=yes,pac_cv_have_float16=no)])
    if test "$pac_cv_have_float16" = "yes" ; then
        AC_DEFINE(HAVE_FLOAT16,1,[Define if _Float16 is supported])
    fi
fi

# ----------------------------------------------------------------------------
AC_CHECK_ALIGNOF([max_align_t],[0],[#include <stddef.h>])

if test "$ac_cv_alignof_max_align_t" != "0" ; then
    AC_DEFINE_UNQUOTED(MAX_ALIGNMENT,$ac_cv_alignof_max_align_t,[Controls byte alignment of structures (for aligning allocated structures)])
else
    AC_CHECK_ALIGNOF(long double)
    AC_DEFINE_UNQUOTED(MAX_ALIGNMENT,$ac_cv_alignof_long_double,[Controls byte alignment of structures (for aligning allocated structures)])
fi

# There are further alignment checks after we test for int64_t etc. below.

# Get the size of the C types for encoding in the basic datatypes and for
# the specific-sized integers
AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(unsigned char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(void *)

# Let's not continue if we cannot detect these basic types
for type in char int void_p ; do
    eval len=\$ac_cv_sizeof_$type
    if test "$len" = "0" ; then
        AC_MSG_ERROR([Could not get size of "$type".])
    fi
done

AC_CHECK_HEADERS([stddef.h])
AC_CHECK_SIZEOF(wchar_t, 0, [
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
])

AC_CHECK_SIZEOF(__float128, 0)
if test "$ac_cv_sizeof___float128" = "16" ; then
    AC_DEFINE(HAVE_FLOAT128, 1, [Define if __float128 is supported])
fi

AC_CHECK_SIZEOF(float_int, 0, [typedef struct { float a; int b; } float_int; ])
AC_CHECK_SIZEOF(double_int, 0, [typedef struct { double a; int b; } double_int; ])
AC_CHECK_SIZEOF(long_int, 0, [typedef struct { long a; int b; } long_int; ])
AC_CHECK_SIZEOF(short_int, 0, [typedef struct { short a; int b; } short_int; ])
AC_CHECK_SIZEOF(two_int, 0, [typedef struct { int a; int b; } two_int; ])
AC_CHECK_SIZEOF(long_double_int, 0, [typedef struct { long double a; int b;} long_double_int; ])

# alignments for basic types
AC_CHECK_ALIGNOF(char)
AC_CHECK_ALIGNOF(float)
AC_CHECK_ALIGNOF(double)
AC_CHECK_ALIGNOF(long)
AC_CHECK_ALIGNOF(long long)
AC_CHECK_ALIGNOF(long double)
AC_CHECK_ALIGNOF(short)
AC_CHECK_ALIGNOF(int)
AC_CHECK_ALIGNOF(bool)
AC_CHECK_ALIGNOF(wchar_t)

# sys/bitypes.h defines the int16_t etc. on some systems (e.g., OSF1).
# Include it when testing for these types
AC_CHECK_HEADER(sys/bitypes.h,[use_bitypes="#include <sys/bitypes.h>"
AC_DEFINE(HAVE_SYS_BITYPES_H,1,[Define if sys/bitypes.h exists])])
# A C99 compliant compiler should have inttypes.h for fixed-size int types
AC_CHECK_HEADERS(inttypes.h stdint.h)

AC_CHECK_SIZEOF(int8_t)
AC_CHECK_SIZEOF(int16_t)
AC_CHECK_SIZEOF(int32_t)
AC_CHECK_SIZEOF(int64_t)

AC_CHECK_SIZEOF(uint8_t)
AC_CHECK_SIZEOF(uint16_t)
AC_CHECK_SIZEOF(uint32_t)
AC_CHECK_SIZEOF(uint64_t)

AC_CHECK_ALIGNOF(int8_t)
AC_CHECK_ALIGNOF(int16_t)
AC_CHECK_ALIGNOF(int32_t)
AC_CHECK_ALIGNOF(int64_t)

if test "$ac_cv_sizeof_int8_t" -eq 1 ; then
    AC_DEFINE(HAVE_INT8_T,1,[Define if int8_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_int16_t" -eq 2 ; then
    AC_DEFINE(HAVE_INT16_T,1,[Define if int16_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_int32_t" -eq 4 ; then
    AC_DEFINE(HAVE_INT32_T,1,[Define if int32_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_int64_t" -eq 8 ; then
    AC_DEFINE(HAVE_INT64_T,1,[Define if int64_t is supported by the C compiler])
fi

if test "$ac_cv_sizeof_uint8_t" -eq 1 ; then
    AC_DEFINE(HAVE_UINT8_T,1,[Define if uint8_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_uint16_t" -eq 2 ; then
    AC_DEFINE(HAVE_UINT16_T,1,[Define if uint16_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_uint32_t" -eq 4 ; then
    AC_DEFINE(HAVE_UINT32_T,1,[Define if uint32_t is supported by the C compiler])
fi
if test "$ac_cv_sizeof_uint64_t" -eq 8 ; then
    AC_DEFINE(HAVE_UINT64_T,1,[Define if uint64_t is supported by the C compiler])
fi

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
AS_IF([test "X$pac_cv_have_long_double" = "Xyes"],[
  AC_CHECK_SIZEOF([long double _Complex],0,[
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif
  ])
])
AC_CHECK_SIZEOF([_Float16])

# we need really could just use the result of AC_CHECK_SIZEOF, but having a
# HAVE_typename macro is useful for consistency
AC_CHECK_TYPES([_Bool, float _Complex, double _Complex])

AS_IF([test "X$pac_cv_have_long_double" = "Xyes"],[
  AC_CHECK_TYPES([long double _Complex])
])

dnl define shell functions e.g. to_hex, to_dec, get_c_int_type, get_c_float_type
PAC_DATATYPE_UTILS()

# Generate a hex version of the size of each type
for type in short int long float double \
    float_int double_int long_int short_int two_int ; do
    eval len=\$ac_cv_sizeof_$type
    if test -z "$len" ; then
        AC_MSG_ERROR([Configure was unable to determine the size of $type ; if cross compiling,
use the --with-cross=file configure option to specify a file containing
Bourne (sh) shell assignments to sizeof values for all datatype types.
The Perl script maint/gen_cross.pl can be run on the target
system to generate an appropriate file for the --with-cross option. You
may also run the script on local system and use the generated file as
template and manually set the correct values.])
    fi
done

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
#    INTEGER_KIND, ADDRESS_KIND, and OFFSET_KIND
#
# ----------------------------------------------------------------------------
if test "$enable_f77" = yes ; then
    pac_cv_f77_sizeof_character=1

    # Up to size checking code in main configure.ac (where it tries to
    # find the matching C sizes) as part of defining mpi_integer8 etc.
    # The results are available in pac_cv_sizeof_f77_<type>
    # Size is 0 if unknown or unavailable (or cross-compiling)
    # Due to limitations in autoconf, we cannot put these into a loop.
    # We also check integer to find the type of MPI_Fint
    #
    # Cross-compilation results can be included with the --with-cross=file
    # option.
    CROSS_F77_SIZEOF_INTEGER=${CROSS_F77_SIZEOF_INTEGER:-0}
    CROSS_F77_SIZEOF_REAL=${CROSS_F77_SIZEOF_REAL:-0}
    CROSS_F77_SIZEOF_DOUBLE_PRECISION=${CROSS_F77_SIZEOF_DOUBLE_PRECISION:-0}
    CROSS_F77_SIZEOF_LOGICAL=${CROSS_F77_SIZEOF_LOGICAL:-0}
    PAC_PROG_F77_CHECK_SIZEOF_EXT(integer,$CROSS_F77_SIZEOF_INTEGER)
    PAC_PROG_F77_CHECK_SIZEOF_EXT(real,$CROSS_F77_SIZEOF_REAL)
    PAC_PROG_F77_CHECK_SIZEOF_EXT(double precision,$CROSS_F77_SIZEOF_DOUBLE_PRECISION)
    PAC_PROG_F77_CHECK_SIZEOF_EXT(logical,$CROSS_F77_SIZEOF_LOGICAL)
    AC_LANG([Fortran 77])
    # If we have sizes for real and double, we do not need to call
    # mpir_get_fsize at run time.
    # For the size-defined types (e.g., integer*2), we assume that if the
    # compiler allows it, it has the stated size.
    AC_CACHE_CHECK([whether integer*1 is supported],pac_cv_fort_integer1,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      integer*1 i])],
         pac_cv_fort_integer1=yes,
         pac_cv_fort_integer1=no)])
    AC_CACHE_CHECK([whether integer*2 is supported],pac_cv_fort_integer2,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      integer*2 i])],
        pac_cv_fort_integer2=yes,
        pac_cv_fort_integer2=no)])
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
    AC_CACHE_CHECK([whether real*4 is supported],pac_cv_fort_real4,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      real*4 a])],
        pac_cv_fort_real4=yes,
        pac_cv_fort_real4=no)])
    AC_CACHE_CHECK([whether real*8 is supported],pac_cv_fort_real8,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      real*8 a])],
        pac_cv_fort_real8=yes,
        pac_cv_fort_real8=no)])
    AC_CACHE_CHECK([whether real*16 is supported],pac_cv_fort_real16,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      real*16 a])],
        pac_cv_fort_real16=yes,
        pac_cv_fort_real16=no)])

    if test -z "$pac_cv_f77_sizeof_integer" -o \
       "X$pac_cv_f77_sizeof_integer" = "X0" ; then
        AC_MSG_ERROR([Unable to configure with Fortran support because configure could not determine the size of a Fortran INTEGER.  Consider setting CROSS_F77_SIZEOF_INTEGER to the length in bytes of a Fortran INTEGER])
    fi

    len_real=`printf "%02x" $pac_cv_f77_sizeof_real`
    MPI_REAL=0x4c00${len_real}1c
    AC_SUBST(MPI_REAL)

    if test -z "$pac_cv_f77_sizeof_double_precision" ; then
        AC_MSG_ERROR([Unable to configure with Fortran support because configure could not determine the size of a Fortran DOUBLE PRECISION.  Consider setting CROSS_F77_SIZEOF_DOUBLE_PRECISION to the length in bytes of a Fortran DOUBLE PRECISION])
    fi

    # Provide the corresponding C types for MPI_INTEGER
    AC_MSG_CHECKING([for C type matching Fortran integer])
    get_c_int_type $pac_cv_f77_sizeof_integer
    MPI_FINT=$pac_retval
    AC_MSG_RESULT($MPI_FINT)

    # Provide the corresponding C types for MPI_REAL and MPI_DOUBLE
    # These are needed to correctly implement the MPI reduction operations

    AC_MSG_CHECKING([for C type matching Fortran real])
    get_c_float_type $pac_cv_f77_sizeof_real
    MPIR_FC_REAL_CTYPE=$pac_retval
    AC_MSG_RESULT($MPIR_FC_REAL_CTYPE)
    AC_DEFINE_UNQUOTED([MPIR_FC_REAL_CTYPE],[$MPIR_FC_REAL_CTYPE], [The C type for FORTRAN REAL])

    AC_MSG_CHECKING([for C type matching Fortran double])
    get_c_float_type $pac_cv_f77_sizeof_double_precision
    MPIR_FC_DOUBLE_CTYPE=$pac_retval
    AC_MSG_RESULT($MPIR_FC_DOUBLE_CTYPE)
    AC_DEFINE_UNQUOTED([MPIR_FC_DOUBLE_CTYPE],[$MPIR_FC_DOUBLE_CTYPE], [The C type for FORTRAN DOUBLE PRECISION])

    # get C types for Fortran fixed-size REAL and INTEGER. If no equivallent C type, set
    # pac_cv_f77_sizeof_real# to 0, so that PAC_SET_MPI_TYPE can skip the type.

    PAC_F77_CHECK_FIXED_REAL(4)
    PAC_F77_CHECK_FIXED_REAL(8)
    PAC_F77_CHECK_FIXED_REAL(16)

    PAC_F77_CHECK_FIXED_INTEGER(1)
    PAC_F77_CHECK_FIXED_INTEGER(2)
    PAC_F77_CHECK_FIXED_INTEGER(4)
    PAC_F77_CHECK_FIXED_INTEGER(8)
    PAC_F77_CHECK_FIXED_INTEGER(16)

    # We also need to check the size of MPI_Aint vs MPI_Fint, and
    # define AINT_LARGER_THAN_FINT if aint is larger (this
    # affects code in MPI_Address)
    if test "$ac_cv_sizeof_void_p" != "0" -a \
        "$ac_cv_sizeof_void_p" -gt "$pac_cv_f77_sizeof_integer" ; then
        AC_DEFINE(HAVE_AINT_LARGER_THAN_FINT,1,[Define if addresses are larger than Fortran integers])
    fi
    if test "$ac_cv_sizeof_void_p" != 0 -a \
        "$ac_cv_sizeof_void_p" != "$pac_cv_f77_sizeof_integer" ; then
	AC_DEFINE(HAVE_AINT_DIFFERENT_THAN_FINT,1,[Define if addresses are a different size than Fortran integers])
    fi

    # Include a defined value for Fint is int
    if test "$pac_cv_f77_sizeof_integer" == "$ac_cv_sizeof_int" ; then
        AC_DEFINE(HAVE_FINT_IS_INT,1,[Define if Fortran integer are the same size as C ints])
    else
        AC_MSG_WARN([Fortran integers and C ints are not the same size.  Support for this case is experimental; use at your own risk])
    fi

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
        # Offset kind should be for 8 bytes if possible (Romio prefers that)
        # address should be sizeof void * (unless --with-aint-size has
        # been set)
        # FIXME in the current configure implementation OFFSET_KIND and
        # MPI_Offset won't always agree, but generally will.  The MPI Standard
        # implies that these types must have identical size, so this is a bug
        # waiting to happen.
	if test "$with_aint_size" -gt 0 -a \
	        "$with_aint_size" -gt "$ac_cv_sizeof_void_p" ; then
	    testsize=$with_aint_size
        else
            testsize=$ac_cv_sizeof_void_p
        fi
        if test "$testsize" = 0 ; then
            # Set a default
            testsize=4
        fi
        dnl Using the {} around testsize helps the comments work correctly
        PAC_PROG_FC_INT_KIND(ADDRESS_KIND,${testsize},$CROSS_F90_ADDRESS_KIND)
        if test "$testsize" = 8 ; then
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
	if test "$pac_cv_fort90_real8" = "yes" ; then
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
    if test "$enable_f90" = "yes" ; then
        if test "$ADDRESS_KIND" -le 0 -o "$OFFSET_KIND" -le 0 ; then
	    AC_MSG_ERROR([Unable to determine Fortran 90 integer kinds for MPI types.  If you do not need Fortran 90, add --disable-fc to the configure options.])
        fi
    fi
    AC_SUBST(ADDRESS_KIND)
    AC_SUBST(OFFSET_KIND)
    AC_SUBST(INTEGER_KIND)

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

    AC_LANG([C])
else
    MPI_FINT=int

    # set following sizes to 0 will disable the corresponding datatype
    pac_cv_f77_sizeof_character=0
    pac_cv_f77_sizeof_integer=0
    pac_cv_f77_sizeof_real=0
    pac_cv_f77_sizeof_double_precision=0
    pac_cv_f77_sizeof_logical=0
    pac_cv_f77_sizeof_integer1=0
    pac_cv_f77_sizeof_integer2=0
    pac_cv_f77_sizeof_integer4=0
    pac_cv_f77_sizeof_integer8=0
    pac_cv_f77_sizeof_integer16=0
    pac_cv_f77_sizeof_real4=0
    pac_cv_f77_sizeof_real8=0
    pac_cv_f77_sizeof_real16=0

    AC_DEFINE(HAVE_NO_FORTRAN_MPI_TYPES_IN_C,1,[Define if the Fortran types are not available in C])
fi
# ----------------------------------------------------------------------------
# C++ types - we support CXX datatypes if there is a working CXX compiler

if test -n "$CXX" ; then
    AC_LANG([C++])
    AC_CHECK_SIZEOF(bool)

    AC_MSG_CHECKING([for C types matching C++ bool])
    get_c_bool_type $ac_cv_sizeof_bool
    AC_MSG_RESULT([$pac_retval])
    AC_DEFINE_UNQUOTED([MPIR_CXX_BOOL_CTYPE],[$pac_retval],
			[a C type used to compute C++ bool reductions])
    if test "$ac_cv_sizeof_bool" != 0 ; then
        AC_DEFINE(HAVE_CXX_BOOL,1,[Define if C++ supports bool types])
    fi

    AC_CHECK_HEADER(complex)
    if test "$ac_cv_header_complex" = "yes" ; then
        # The C++ complex types are all templated.  We finagle this by
        # defining a standin name
        AC_CHECK_SIZEOF(Complex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define Complex complex<float>
])
        AC_CHECK_SIZEOF(DoubleComplex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define DoubleComplex complex<double>
])
        if test "$MPID_NO_LONG_DOUBLE" != yes ; then
             AC_CHECK_SIZEOF(LongDoubleComplex,0,[#include <stdio.h>
#include <complex>
using namespace std;
#define LongDoubleComplex complex<long double>
])
        fi

        # If either complex or double complex have length 0, then mark
        # c++ complex as unavailable
        if test "$ac_cv_sizeof_Complex" != 0 -a \
                "$ac_cv_sizeof_DoubleComplex" != 0 ; then
            AC_DEFINE(HAVE_CXX_COMPLEX,1,[Define if C++ supports complex types])
        fi
        # mark availability of c++ long double complex
        if test $ac_cv_sizeof_long_double__Complex != 0 ; then
            AC_DEFINE([HAVE_CXX_LONG_DOUBLE_COMPLEX], 1, [Define if C++ supports long double complex])
        fi

    fi

    AC_LANG([C])
else
    # set following sizes to 0 to disable the corresponding datatypes
    ac_cv_sizeof_bool=0
    ac_cv_sizeof_Complex=0
    ac_cv_sizeof_DoubleComplex=0
    ac_cv_sizeof_LongDoubleComplex=0
fi

# ------------
# MPI_FINT
# ------------
# MPI_FINT is already set
AC_SUBST(MPI_FINT)

# ------------
# MPI_AINT
# ------------
# Note that aint_size must be used instead of void_p where the desired check
# is on the size of MPI_Aint
aint_size=$ac_cv_sizeof_void_p
if test "$with_aint_size" -gt 0 ; then
   aint_size=$with_aint_size
   if test "$aint_size" != "$ac_cv_sizeof_void_p" ; then
        AC_MSG_RESULT([Overriding MPI_Aint to be $aint_size bytes])
   fi
fi
if test "$ac_cv_sizeof_void_p" -lt "$aint_size" ; then
    AC_DEFINE(USE_AINT_FOR_ATTRVAL,1,[Define if MPI_Aint should be used instead of void * for storing attribute values])
fi

get_c_int_type $aint_size
MPI_AINT=$pac_retval
MPI_SIZEOF_AINT=$aint_size
export MPI_SIZEOF_AINT

case $MPI_AINT in
    int)
        MPI_AINT_FMT_DEC_SPEC="%d"
        MPI_AINT_FMT_HEX_SPEC="%x"
        MPIR_AINT_MAX="INT_MAX"
        MPIR_AINT_MIN="INT_MIN"
        ;;
    long)
        MPI_AINT_FMT_DEC_SPEC="%ld"
        MPI_AINT_FMT_HEX_SPEC="%lx"
        MPIR_AINT_MAX="LONG_MAX"
        MPIR_AINT_MIN="LONG_MIN"
        ;;
    long_long)
        MPI_AINT_FMT_DEC_SPEC="%lld"
        MPI_AINT_FMT_HEX_SPEC="%llx"
        # tt#1776: if LLONG_MAX is missing, we fix it up in C, b/c it's
        # easier there.  See mpiiimpl.h.
        MPIR_AINT_MAX="LLONG_MAX"
        MPIR_AINT_MIN="LLONG_MIN"
        ;;
    short)
        MPI_AINT_FMT_DEC_SPEC="%hd"
        MPI_AINT_FMT_HEX_SPEC="%hx"
        MPIR_AINT_MAX="SHRT_MAX"
        MPIR_AINT_MIN="SHRT_MIN"
        ;;
    *)
        AC_MSG_WARN([unable to determine format specifiers for MPI_Aint, defaulting to int])
        MPI_AINT_FMT_DEC_SPEC="%d"
        MPI_AINT_FMT_HEX_SPEC="%x"
        MPIR_AINT_MAX="INT_MAX"
        MPIR_AINT_MIN="INT_MIN"
    ;;
esac
export MPI_AINT_FMT_DEC_SPEC MPI_AINT_FMT_HEX_SPEC

AC_SUBST(MPI_AINT)
AC_SUBST(MPI_AINT_FMT_DEC_SPEC)
AC_SUBST(MPI_AINT_FMT_HEX_SPEC)
AC_DEFINE_UNQUOTED([MPIR_AINT_MAX],[$MPIR_AINT_MAX],[limits.h _MAX constant for MPI_Aint])
AC_DEFINE_UNQUOTED([MPIR_AINT_MIN],[$MPIR_AINT_MIN],[limits.h _MIN constant for MPI_Aint])

# ------------
# MPI_OFFSET
# ------------
# If ROMIO was successfully configured, then ROMIO will have exported the
# definition of MPI_OFFSET_TYPE through its localdefs file (created by the
# ROMIO configure in src/mpi/romio/localdefs).  If MPI_OFFSET_TYPE was not
# defined, this code attempts to find a good choice for MPI_OFFSET_TYPE
# (As the offset type is used for File operations, the specific type
# really doesn't matter if ROMIO doesn't provide it).
if test -n "$MPI_OFFSET_TYPE" ; then
    # We got the value from the ROMIO configure
    MPI_OFFSET="$MPI_OFFSET_TYPE"
    # Get and export the size of this type if possible
    if test -z "$MPI_SIZEOF_OFFSET" ; then
        # set a default
        AC_CACHE_CHECK([the sizeof MPI_Offset],ac_cv_sizeof_MPI_Offset,[
            ac_cv_sizeof_MPI_Offset=unknown
            AC_COMPUTE_INT([ac_cv_sizeof_MPI_Offset],[sizeof($MPI_OFFSET)],[],[
                AC_MSG_WARN([Unable to determine the size of MPI_Offset])
            ])
	])
	if test "$ac_cv_sizeof_MPI_Offset" != "unknown" ; then
  	    MPI_SIZEOF_OFFSET=$ac_cv_sizeof_MPI_Offset
        fi
    fi
    export MPI_SIZEOF_OFFSET
else
    # Make a guess at the appropriate definition for offset.  Try to
    # find a 64bit type.
    if test "$ac_cv_sizeof_long" = 8 ; then
        MPI_OFFSET="long"
	# Make the size of this type available to other configures
	MPI_SIZEOF_OFFSET=8
    elif test "$ac_cv_sizeof_long_long" = 8 ; then
        MPI_OFFSET="long long"
	# Make the size of this type available to other configures
	MPI_SIZEOF_OFFSET=8
    else
        MPI_OFFSET=long
        MPI_SIZEOF_OFFSET=$ac_cv_sizeof_long
    fi
    export MPI_SIZEOF_OFFSET
fi
AC_SUBST(MPI_OFFSET)

AS_CASE([$MPI_OFFSET],
        [int],         [MPIR_OFFSET_MAX="INT_MAX"],
        [long],        [MPIR_OFFSET_MAX="LONG_MAX"],
        ['long long'], [MPIR_OFFSET_MAX="LLONG_MAX"],
        [short],       [MPIR_OFFSET_MAX="SHRT_MAX"],
        [AC_MSG_ERROR([unable to determine MPIR_OFFSET_MAX for MPI_Offset])])
AC_DEFINE_UNQUOTED([MPIR_OFFSET_MAX],[$MPIR_OFFSET_MAX],[limits.h _MAX constant for MPI_Offset])

# FIXME: we need an explanation of why we need both MPI_OFFSET and
# MPI_OFFSET_TYPEDEF.   Why is MPI_OFFSET_TYPEDEF necessary?
# This appears to be used by the Windows "winconfigure.wsf" which is used
# to create a multiline definition using an #ifdef check on USE_GCC
# We may wish to use a different approach
MPI_OFFSET_TYPEDEF="typedef $MPI_OFFSET MPI_Offset;"
AC_SUBST(MPI_OFFSET_TYPEDEF)
#
# Fortran type for an Offset type (needed to define MPI_DISPLACEMENT_CURRENT
# The value for this comes from ROMIO, and is needed in mpif.h.in
# First, we check that this works with both Fortran compilers (if
# they are defined)
#
# If there is no FORTRAN_MPI_OFFSET type (because ROMIO is disabled),
# just use INTEGER
if test -z "$FORTRAN_MPI_OFFSET" ; then
    FORTRAN_MPI_OFFSET=INTEGER
fi
if test "$enable_f77" = yes -a "$enable_f90" = yes ; then
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

# ------------
# MPI_COUNT
# ------------
# quick sanity checking to avoid a bad test immediately below
AS_IF([test -z "$MPI_SIZEOF_AINT"],
      [AC_MSG_ERROR([size of MPI_Aint is unknown at this stage])])
AS_IF([test -z "$MPI_SIZEOF_OFFSET"],
      [AC_MSG_ERROR([size of MPI_Offset is unknown at this stage])])

AS_IF([test "$MPI_SIZEOF_AINT" -gt "$MPI_SIZEOF_OFFSET"],
      [# an unlikely case, but I suppose it's theoretically possible
       MPI_COUNT="$MPI_AINT"
       COUNT_KIND="$ADDRESS_KIND"
       MPIR_COUNT_MAX="$MPIR_AINT_MAX"
       MPI_SIZEOF_COUNT="$MPI_SIZEOF_AINT"],
      [# don't bother checking whether Aint or Offset are larger than int, they
       # surely will be
       MPI_COUNT="$MPI_OFFSET"
       COUNT_KIND="$OFFSET_KIND"
       MPIR_COUNT_MAX="$MPIR_OFFSET_MAX"
       MPI_SIZEOF_COUNT="$MPI_SIZEOF_OFFSET"])
AC_SUBST([MPI_COUNT])
AC_SUBST([COUNT_KIND])
AC_DEFINE_UNQUOTED([MPIR_COUNT_MAX],[$MPIR_COUNT_MAX],[limits.h _MAX constant for MPI_Count])
AC_DEFINE_UNQUOTED(MPIR_Ucount,unsigned $MPI_COUNT,[MPIR_Ucount is an unsigned MPI_Count-sized integer])

#--------------------------------------------------------------------------------
# Now we collected all the datatype sizes, we can set values for MPI builtin datatypes
AC_MSG_CHECKING([MPI datatypes])
# PAC_SET_MPI_TYPE(hex_idx, NAME, len)
PAC_SET_MPI_TYPE(01, MPI_CHAR, 1)
PAC_SET_MPI_TYPE(02, MPI_UNSIGNED_CHAR, 1)
PAC_SET_MPI_TYPE(03, MPI_SHORT, $ac_cv_sizeof_short)
PAC_SET_MPI_TYPE(04, MPI_UNSIGNED_SHORT, $ac_cv_sizeof_short)
PAC_SET_MPI_TYPE(05, MPI_INT, $ac_cv_sizeof_int)
PAC_SET_MPI_TYPE(06, MPI_UNSIGNED, $ac_cv_sizeof_int)
PAC_SET_MPI_TYPE(07, MPI_LONG, $ac_cv_sizeof_long)
PAC_SET_MPI_TYPE(08, MPI_UNSIGNED_LONG, $ac_cv_sizeof_long)
PAC_SET_MPI_TYPE(09, MPI_LONG_LONG_INT, $ac_cv_sizeof_long_long)
PAC_SET_MPI_TYPE(0a, MPI_FLOAT, $ac_cv_sizeof_float)
PAC_SET_MPI_TYPE(0b, MPI_DOUBLE, $ac_cv_sizeof_double)
PAC_SET_MPI_TYPE(0c, MPI_LONG_DOUBLE, $ac_cv_sizeof_long_double)
PAC_SET_MPI_TYPE(0d, MPI_BYTE, 1)
PAC_SET_MPI_TYPE(0e, MPI_WCHAR, $ac_cv_sizeof_wchar_t)
PAC_SET_MPI_TYPE(0f, MPI_PACKED, 1)
PAC_SET_MPI_LBUB(10, MPI_LB)
PAC_SET_MPI_LBUB(11, MPI_UB)

PAC_SET_MPI_PAIRTYPE(00, MPI_FLOAT_INT)
PAC_SET_MPI_PAIRTYPE(01, MPI_DOUBLE_INT)
PAC_SET_MPI_PAIRTYPE(02, MPI_LONG_INT)
PAC_SET_MPI_PAIRTYPE(03, MPI_SHORT_INT)
PAC_SET_MPI_TYPE(16, MPI_2INT, $ac_cv_sizeof_two_int)
PAC_SET_MPI_PAIRTYPE(04, MPI_LONG_DOUBLE_INT)
PAC_SET_MPI_TYPE(18, MPI_SIGNED_CHAR, 1)
PAC_SET_MPI_TYPE(19, MPI_UNSIGNED_LONG_LONG, $ac_cv_sizeof_long_long)
PAC_SET_MPI_TYPE(1a, MPI_CHARACTER, $pac_cv_f77_sizeof_character)
PAC_SET_MPI_TYPE(1b, MPI_INTEGER, $pac_cv_f77_sizeof_integer)
PAC_SET_MPI_TYPE(1c, MPI_REAL, $pac_cv_f77_sizeof_real)
PAC_SET_MPI_TYPE(1d, MPI_LOGICAL, $pac_cv_f77_sizeof_logical)
PAC_SET_MPI_TYPE(1e, MPI_COMPLEX, $pac_cv_f77_sizeof_real, 2)
PAC_SET_MPI_TYPE(1f, MPI_DOUBLE_PRECISION, $pac_cv_f77_sizeof_double_precision)
PAC_SET_MPI_TYPE(20, MPI_2INTEGER, $pac_cv_f77_sizeof_integer, 2)
PAC_SET_MPI_TYPE(21, MPI_2REAL, $pac_cv_f77_sizeof_real, 2)
PAC_SET_MPI_TYPE(22, MPI_DOUBLE_COMPLEX, $pac_cv_f77_sizeof_double_precision, 2)
PAC_SET_MPI_TYPE(23, MPI_2DOUBLE_PRECISION, $pac_cv_f77_sizeof_double_precision, 2)
PAC_SET_MPI_TYPE(24, MPI_2COMPLEX, $pac_cv_f77_sizeof_real, 4)
PAC_SET_MPI_TYPE(25, MPI_2DOUBLE_COMPLEX, $pac_cv_f77_sizeof_double_precision, 4)

PAC_SET_MPI_TYPE(27, MPI_REAL4, $pac_cv_f77_sizeof_real4)
PAC_SET_MPI_TYPE(28, MPI_COMPLEX8, $pac_cv_f77_sizeof_real4, 2)
PAC_SET_MPI_TYPE(29, MPI_REAL8, $pac_cv_f77_sizeof_real8)
PAC_SET_MPI_TYPE(2a, MPI_COMPLEX16, $pac_cv_f77_sizeof_real8, 2)
PAC_SET_MPI_TYPE(2b, MPI_REAL16, $pac_cv_f77_sizeof_real16)
PAC_SET_MPI_TYPE(2c, MPI_COMPLEX32, $pac_cv_f77_sizeof_real16, 2)
PAC_SET_MPI_TYPE(2d, MPI_INTEGER1, $pac_cv_f77_sizeof_integer1)

PAC_SET_MPI_TYPE(2f, MPI_INTEGER2, $pac_cv_f77_sizeof_integer2)
PAC_SET_MPI_TYPE(30, MPI_INTEGER4, $pac_cv_f77_sizeof_integer4)
PAC_SET_MPI_TYPE(31, MPI_INTEGER8, $pac_cv_f77_sizeof_integer8)
PAC_SET_MPI_TYPE(32, MPI_INTEGER16, $pac_cv_f77_sizeof_integer16)
PAC_SET_MPI_TYPE(33, MPIR_CXX_BOOL, $ac_cv_sizeof_bool)
PAC_SET_MPI_TYPE(34, MPIR_CXX_FLOAT_COMPLEX, $ac_cv_sizeof_Complex)
PAC_SET_MPI_TYPE(35, MPIR_CXX_DOUBLE_COMPLEX, $ac_cv_sizeof_DoubleComplex)
PAC_SET_MPI_TYPE(36, MPIR_CXX_LONG_DOUBLE_COMPLEX, $ac_cv_sizeof_LongDoubleComplex)
PAC_SET_MPI_TYPE(37, MPI_INT8_T, 1)
PAC_SET_MPI_TYPE(38, MPI_INT16_T, 2)
PAC_SET_MPI_TYPE(39, MPI_INT32_T, 4)
PAC_SET_MPI_TYPE(3a, MPI_INT64_T, 8)
PAC_SET_MPI_TYPE(3b, MPI_UINT8_T, 1)
PAC_SET_MPI_TYPE(3c, MPI_UINT16_T, 2)
PAC_SET_MPI_TYPE(3d, MPI_UINT32_T, 4)
PAC_SET_MPI_TYPE(3e, MPI_UINT64_T, 8)
PAC_SET_MPI_TYPE(3f, MPI_C_BOOL, $ac_cv_sizeof__Bool)
PAC_SET_MPI_TYPE(40, MPI_C_FLOAT_COMPLEX, $ac_cv_sizeof_float__Complex)
PAC_SET_MPI_TYPE(41, MPI_C_DOUBLE_COMPLEX, $ac_cv_sizeof_double__Complex)
PAC_SET_MPI_TYPE(42, MPI_C_LONG_DOUBLE_COMPLEX, $ac_cv_sizeof_long_double__Complex)
PAC_SET_MPI_TYPE(43, MPI_AINT_DATATYPE, $MPI_SIZEOF_AINT)
PAC_SET_MPI_TYPE(44, MPI_OFFSET_DATATYPE, $MPI_SIZEOF_OFFSET)
PAC_SET_MPI_TYPE(45, MPI_COUNT_DATATYPE, $MPI_SIZEOF_COUNT)
PAC_SET_MPI_TYPE(46, MPIX_C_FLOAT16, 2)
AC_MSG_RESULT([done])

# aliases
PAC_SET_MPI_DATATYPE_ALIAS(MPI_LONG_LONG, MPI_LONG_LONG_INT)
PAC_SET_MPI_DATATYPE_ALIAS(MPI_C_COMPLEX, MPI_C_FLOAT_COMPLEX)

# ------------------------------------------------------------------------
# Test if  type_tag_for_datatype is agnostic to modifiers such as const, volatile, and restrict
# if not, the type tags are disabled to avoid compiler warings. A new type_tag for const etc. does
# does not help, since only the latest type_tag definition is used. Type tags are defined in mpi.h,
# therefore, they must be also be activated/deavtivated there
PAC_PUSH_FLAG([CFLAGS])
PAC_C_CHECK_COMPILER_OPTION([-Werror=type-safety],[CFLAGS="$CFLAGS -Werror=type-safety"])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    typedef int TEST_Datatype;
    #define TEST_INT ((TEST_Datatype)0x44)
    static const TEST_Datatype test_int  __attribute__((type_tag_for_datatype(TEST,int)))  = TEST_INT;
    void test(const void* buffer, TEST_Datatype datatype)__attribute__((pointer_with_type_tag(TEST,1,2)));
    ]],[[
    const int buf[10];
    test(buf, TEST_INT);
    ]])],ac_attr_support=yes, ac_attr_support=no)
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
        AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
        ]])],pac_cv_int64_t_alignment=no,pac_cv_int64_t_alignment=yes,pac_cv_int64_t_alignment=$default_int64_t_alignment)
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
        AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
        ]])],pac_cv_int32_t_alignment=no,pac_cv_int32_t_alignment=yes,pac_cv_int32_t_alignment=$default_int32_t_alignment)
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
# Check for a specific header
# Grrr.  OS/X fails the test for sys/uio.h because uio *requires* sys/types.h
# to compile.  Thus, we'll make that a separate test
# stddef.h is sometimes needed for types like wchar_t
AC_CHECK_HEADERS(stdlib.h stdarg.h sys/types.h string.h inttypes.h limits.h stddef.h errno.h sys/socket.h sys/time.h unistd.h endian.h assert.h sys/param.h)
AC_CACHE_CHECK([for sys/uio.h],ac_cv_header_sys_uio_h,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <sys/types.h>
        #include <sys/uio.h>
        ]],[[
        int a;
        ]])],ac_cv_header_sys_uio_h=yes,ac_cv_header_sys_uio_h=no)
])
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
# in MPIR_Strerror
AC_CHECK_FUNCS(strerror strncasecmp)
AC_FUNC_STRERROR_R
if test "$ac_cv_func_strerror_r" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strerror_r)
fi

# qsort will be used in MPI_Comm_split, if available
AC_CHECK_FUNCS([qsort])

PAC_C_MACRO_VA_ARGS

if test "$enable_g_mem" != "yes" ; then
    # Strdup is needed only if memory tracing is not enabled.
    AC_CHECK_FUNCS(strdup)
    if test "$ac_cv_func_strdup" = "yes" ; then
        # Do we need to declare strdup?
        PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
    fi
fi

# make sure we support signal
AC_CHECK_HEADERS(signal.h)
AC_CHECK_FUNCS(signal)

# ----------------------------------------------------------------------------
# Look for some non-posix, but commonly provided functions
# ----------------------------------------------------------------------------
# putenv() sets environment variable
AC_CHECK_FUNCS(putenv)
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

# with MPI_AINT defined, now we can
# Get the size for the bsendoverhead
AC_CHECK_SIZEOF(MPII_Bsend_data_t,0,[
#define MPI_Datatype int
typedef $MPI_AINT MPI_Aint;

struct MPIR_Request;
typedef struct MPIR_Request MPIR_Request;

struct MPIR_Comm;
typedef struct MPIR_Comm MPIR_Comm;

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "${main_top_srcdir}/src/include/mpir_bsend.h"]
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
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sched.h>]],[[cpu_set_t t;]])],
        pac_cv_have_cpu_set_t=yes,pac_cv_have_cpu_set_t=no)])
    if test "$pac_cv_have_cpu_set_t" = yes ; then
        AC_DEFINE(HAVE_CPU_SET_T,1,[Define if cpu_set_t is defined in sched.h])

	AC_CACHE_CHECK([whether the CPU_SET and CPU_ZERO macros are defined],
	pac_cv_cpu_set_defined,[
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sched.h>]],[[cpu_set_t t; CPU_ZERO(&t); CPU_SET(1,&t);]])],
            pac_cv_cpu_set_defined=yes,pac_cv_cpu_set_defined=no)])
	if test "$pac_cv_cpu_set_defined" = "yes" ; then
	    AC_DEFINE(HAVE_CPU_SET_MACROS,1,[Define if CPU_SET and CPU_ZERO defined])
        fi
	# FIXME: Some versions of sched_setaffinity return ENOSYS (!),
	# so we should test for the unfriendly and useless behavior
    fi
fi
if test "$ac_cv_func_thread_policy_set" = yes ; then
    AC_CACHE_CHECK([whether thread affinity macros defined], pac_cv_have_thread_affinity_policy,[
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
            #include <mach/thread_policy.h>
            ]],[[
            #if !defined(THREAD_AFFINITY_POLICY) || !defined(THREAD_AFFINITY_TAG_NULL)
                :'thread macros not defined
            ]])],
            pac_cv_have_thread_affinity_policy=yes,
            pac_cv_have_thread_affinity_policy=no)
    ])
    if test "$pac_cv_have_thread_affinity_policy" = yes ; then
        AC_DEFINE(HAVE_OSX_THREAD_AFFINITY,1,[Define if the OSX thread affinity policy macros defined])
    fi
fi

# -----------------------------------------------------------------------------
# End of thread configure
# -----------------------------------------------------------------------------


# Checkpointing
AC_ARG_ENABLE(checkpointing,
    [AS_HELP_STRING([--enable-checkpointing], [Enable application checkpointing])],
    [ if test "$enableval" != "no" ; then
	PAC_CHECK_HEADER_LIB_FATAL(blcr, libcr.h, cr, cr_init)
        AC_DEFINE(ENABLE_CHECKPOINTING,1,[Application checkpointing enabled])
      fi ],
)

# NVCC can be used to compile examples/cuda/cudapi.cu
if test -z "$NVCC" ; then
    if test -n "${with_cuda}" -a "$with_cuda" != "no" ; then
        AC_PATH_PROG([NVCC], [nvcc], [nvcc_not_found], [$with_cuda/bin:$PATH])
    else
        AC_PATH_PROG([NVCC], [nvcc], [nvcc_not_found])
    fi
fi

# Update the cache first with the results of the previous configure steps
# We don't use the subdir cache because ensuring that the cache is consistent
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
    PAC_CONFIG_SUBDIR_ARGS([$subsys],[$subsys_config_args],[],[AC_MSG_ERROR([$subsys configure failed])])
done
if test "$DEBUG_SUBDIR_CACHE" = yes -a "$enable_echo" != yes ; then
    set +x
fi
dnl PAC_SUBDIR_CACHE_CLEANUP

# Make subsystems available to makefiles.
# FIXME does the makefile actually need this?
subsystems="$devsubsystems $subsystems $bindingsubsystems"

# MPI_STATUS_SIZE is hardcoded and need to be synchronized with mpi.h
MPI_STATUS_SIZE=5
AC_SUBST([MPI_STATUS_SIZE])

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

AM_CONDITIONAL([BUILD_CXX_BINDING],[test "$enable_cxx" = "yes"])
AM_CONDITIONAL([BUILD_F77_BINDING],[test "$enable_f77" = "yes"])
dnl FIXME DJG this has been moved to the f90 bindings subconfigure.m4 for now
dnl AM_CONDITIONAL([BUILD_FC_BINDING],[test "$enable_f90" = "yes"])
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

dnl
dnl Generate the Makefiles from Makefile.in
dnl Also generate mpi.h from mpi.h.in so that we can eliminate all ifdefs
dnl from the file.
dnl
dnl Run a setup command for any external modules (normally, this is empty)
dnl Pass a subset of the environment to the invoked process.
AC_CONFIG_COMMANDS([external_setups],[
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
AC_CONFIG_COMMANDS([lib/newconfig],[
    mkdir -p lib
    date > lib/newconfig
])

AC_CONFIG_COMMANDS([chmod],[chmod a+x test/commands/cmdtests])


AC_DEFINE(HAVE_MPICHCONF,1,[Define so that we can test whether the mpichconf.h file has been included])

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
            # for all three compilers (CC/CXX/FC).  But we only think this is
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

if test "$enable_f08" = "yes"; then
    AS_CASE([$MPI_AINT],
        [short],       [F08_C_AINT="c_short"],
        [int],         [F08_C_AINT="c_int"],
        [long],        [F08_C_AINT="c_long"],
        ['long long'], [F08_C_AINT="c_long_long"],
        [AC_MSG_ERROR([unable to determine C MPI_AINT type in Fortran 2008])])

    AS_CASE([$MPI_COUNT],
        [short],       [F08_C_COUNT="c_short"],
        [int],         [F08_C_COUNT="c_int"],
        [long],        [F08_C_COUNT="c_long"],
        ['long long'], [F08_C_COUNT="c_long_long"],
        [AC_MSG_ERROR([unable to determine C MPI_COUNT type in Fortran 2008])])

    AS_CASE([$MPI_OFFSET],
        [short],       [F08_C_OFFSET="c_short"],
        [int],         [F08_C_OFFSET="c_int"],
        [long],        [F08_C_OFFSET="c_long"],
        ['long long'], [F08_C_OFFSET="c_long_long"],
        [AC_MSG_ERROR([unable to determine C MPI_OFFSET type in Fortran 2008])])

    AC_SUBST(F08_C_AINT)
    AC_SUBST(F08_C_COUNT)
    AC_SUBST(F08_C_OFFSET)
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

if test "$enable_f90" = "yes" ; then
    PAC_FC_CHECK_IGNORE_TKR
    if test -z "$PYTHON" ; then
        if test -f src/binding/fortran/use_mpi/mpi_base.f90 ; then
            AC_MSG_NOTICE([Use pre-generated Fortran mpi binding. To prevent issues, install Python 3 and re-run configure.])
        else
            AC_MSG_ERROR([Python 3 is required to generate F90 bindings but not found!])
        fi
    else
        cmd_f90="$PYTHON $srcdir/maint/gen_binding_f90.py"
        # pass in type sizes for MPI_SIZEOF
        cmd_f90="$cmd_f90 -f-logical-size=$pac_cv_f77_sizeof_logical"
        if test "$pac_fc_ignore_tkr" != "no" ; then
            cmd_f90="$cmd_f90 -ignore-tkr=$pac_fc_ignore_tkr"
        fi
        AC_CONFIG_COMMANDS([gen_binding_f90], [$cmd_gen_binding_f90], [cmd_gen_binding_f90="$cmd_f90"])
    fi
fi

if test "$enable_f08" = "yes" ; then
    PAC_FC_CHECK_REAL128
    if test -z "$PYTHON" ; then
        if test -f src/binding/fortran/use_mpi_f08/mpi_f08.f90 ; then
            AC_MSG_NOTICE([Using pre-generated Fortran mpi_f08 binding source. To prevent issues, install Python 3 and rerun configure.])
        else
            AC_MSG_ERROR([Python 3 is required to generate F08 bindings but not found!])
        fi
    else
        cmd_f08="$PYTHON $srcdir/maint/gen_binding_f08.py"
        if test "$pac_fc_has_real128" = "no" ; then
            cmd_f08="$cmd_f08 -no-real128"
        fi
        if test "$enable_romio" = "no" ; then
            cmd_f08="$cmd_f08 -no-mpiio"
        fi
        cmd_f08="$cmd_f08 -fint-size=$pac_cv_f77_sizeof_integer -aint-size=$MPI_SIZEOF_AINT -count-size=$MPI_SIZEOF_COUNT -cint-size=$ac_cv_sizeof_int"
        AC_CONFIG_COMMANDS([gen_binding_f08], [$cmd_gen_binding_f08], [cmd_gen_binding_f08="$cmd_f08"])
    fi
fi

case "$host_os" in
    freebsd*)
        AC_DEFINE(DELAY_SHM_MUTEX_DESTROY, 1, [Define to workaround interprocess mutex issue on FreeBSD])
        ;;
esac

dnl This includes an experimental pkgconfig file for ch3 in the src/pkgconfig
dnl directory
AC_CONFIG_FILES([Makefile \
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
	  src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90 \
	  src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90 \
	  src/packaging/pkgconfig/mpich.pc \
	  src/packaging/envmods/mpich.module \
          src/env/mpixxx_opts.conf \
          src/env/mpicc.sh \
	  src/env/mpicc.bash \
          src/env/mpicxx.sh \
	  src/env/mpicxx.bash \
	  src/env/mpifort.sh \
	  src/env/mpifort.bash \
          src/env/parkill \
          src/include/mpi.h \
          doc/installguide/Makefile \
          doc/refman/Makefile \
          doc/userguide/Makefile \
          test/commands/cmdtests])
AC_OUTPUT

echo '*****************************************************'
if test ${device_name} = "ch4" ; then
    PAC_CH4_CONFIG_SUMMARY()
elif test ${DEVICE} = "ch3:nemesis" ; then
cat <<EOF
***
*** device configuration: ch3:nemesis
*** nemesis networks: ${nemesis_networks}
***
EOF
else
cat <<EOF
***
*** device configuration: ${DEVICE}
***
EOF
fi
echo '*****************************************************'

echo 'Configuration completed.'
