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
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PAC_PREFIX_ALL_FLAGS(WRAPPER)

# When build from MPICH, --with-mpi=mpich to build native mpich ABI, --with-mpi=abi to build MPI ABI
# When build stand-alone, set --with-mpi to path. TODO: check for system mpi.
AC_ARG_WITH(mpi, [AS_HELP_STRING([--with-mpi=dir],
            [Specify the path where MPI is installed.])],,with_mpi=mpich)

mpi_h=
AC_ARG_VAR([MPILIBNAME], [default: "mpi"])
AC_ARG_VAR([LPMPILIBNAME], [default: ""])
AC_SUBST(MPILIBNAME)
AC_SUBST(LPMPILIBNAME)

if test "$FROM_MPICH" = "yes" ; then
    if test "$WITH_MPI" = "mpich" ; then
        # We expect the following vars to be set from parent configure
        # MPI_MAX_PROCESSOR_NAME, MPI_MAX_LIBRARY_VERSION_STRING, MPI_MAX_ERROR_STRING, BSEND_OVERHEAD
        # MPI_SIZEOF_AINT, MPI_SIZEOF_OFFSET, MPI_SIZEOF_COUNT
        PAC_APPEND_FLAG(-I${main_top_srcdir}/src/include, [CPPFLAGS])
        mpi_h="$main_top_srcdir/src/include/mpi_mpich.h.in"
    elif test "$WITH_MPI" = "abi" ; then
        PAC_APPEND_FLAG(-I${main_top_srcdir}/src/binding, [CPPFLAGS])
        mpi_h="$main_top_srcdir/src/binding/mpi_abi.h"
    else
        AC_MSG_ERROR([$FROM_MPICH is set but $WITH_MPI is not set!])
    fi
    if test -z "$MPILIBNAME" ; then
        AC_MSG_ERROR([MPICH configure need export MPILIBNAME and LPMPILIBNAME])
    fi
else
    if ! test "$with_mpi/include/mpi.h" ; then
        AC_MSG_ERROR([mpi.h not found in $with_mpi/include/])
    fi
    PAC_APPEND_FLAG(-I${with_mpi}/include, [CPPFLAGS])
    PAC_APPEND_FLAG(-L${with_mpi}/lib, [LDFLAGS])
    mpi_h="$with_mpi/include/mpi.h"

    if test -e "$with_mpi/lib/libmpi.so" ; then
        if test -z "$MPILIBNAME" ; then
            MPILIBNAME="mpi"
        fi
        if test -e "$with_mpi/lib/libpmpi.so" ; then
            LPMPILIBNAME="-lpmpi"
        fi
    elif test -e "$with_mpi/lib/libmpi_abi.so" ; then
        if test -z "$MPILIBNAME" ; then
            MPILIBNAME="mpi_abi"
        fi
        if test -e "$with_mpi/lib/libpmpi_abi.so" ; then
            LPMPILIBNAME="-lpmpi_abi"
        fi
    else
        if test -z "$MPILIBNAME" ; then
            AC_MSG_ERROR([Please export \$MPILIBNAME and \$LPMPILIBNAME])
        fi
    fi
fi

# Find a C compiler.
# We also need to do this before the F77 and FC test to ensure that we
# find the C preprocessor reliably.
AC_PROG_CC
AM_PROG_CC_C_O

# check some constants from mpi.h
if test -n "$mpi_h" ; then
    AC_CHECK_SIZEOF(MPI_Aint, 0, [#include "mpi.h"])
    if test $ac_cv_sizeof_MPI_Aint -gt "0" ; then
        MPI_SIZEOF_AINT=$ac_cv_sizeof_MPI_Aint
    fi

    AC_CHECK_SIZEOF(MPI_Offset, 0, [#include "mpi.h"])
    if test $ac_cv_sizeof_MPI_Offset -gt "0" ; then
        MPI_SIZEOF_OFFSET=$ac_cv_sizeof_MPI_Offset
    fi

    AC_CHECK_SIZEOF(MPI_Count, 0, [#include "mpi.h"])
    if test $ac_cv_sizeof_MPI_Count -gt "0" ; then
        MPI_SIZEOF_COUNT=$ac_cv_sizeof_MPI_Count
    fi
fi

if test -z "$MPI_SIZEOF_AINT" ; then
    AC_MSG_ERROR([Can't determine the size of MPI_Aint])
fi
if test -z "$MPI_SIZEOF_OFFSET" ; then
    AC_MSG_ERROR([Can't determine the size of MPI_Offset])
fi
if test -z "$MPI_SIZEOF_COUNT" ; then
    AC_MSG_ERROR([Can't determine the size of MPI_Count])
fi


dnl enable_fortran is assumed to be true

# Add MPICHLIB_* to the appropriate flags
AC_ARG_VAR(MPICHLIB_FFLAGS, [extra FFLAGS used in building MPICH libraries])
AC_ARG_VAR(MPICHLIB_FCFLAGS, [extra FCFLAGS used in building MPICH libraries])
FFLAGS="$FFLAGS $MPICHLIB_FFLAGS"
FCFLAGS="$FCFLAGS $MPICHLIB_FCFLAGS"

# suppress default "-g -O2" from AC_PROG_FC
: ${FCFLAGS=""}

# We only supports a single compiler for FC and F77
AC_PROG_FC

if test "$FC" = "" -o "$FC" = "no"; then
    # No Fortran compiler found; abort
    AC_MSG_ERROR([No Fortran compiler found. If you don't need to
    build any Fortran programs, you can disable Fortran support using
    --disable-fortran. If you do want to build Fortran
    programs, you need to install a Fortran compiler such as gfortran
    or ifort before you can proceed. Set FC to the path of Fortran compiler.])
fi

F77=$FC
FFLAGS=$FCFLAGS
AC_PROG_F77

dnl --enable-g will affect the flags
AC_ARG_ENABLE(g,
[  --enable-g=option - Control the level of debugging support. Accepted options:
        none  - No debugging
        dbg   - Add compiler flag -g
        asan  - Add compiler flag -fsanitize=address
        ubsan - Add compiler flag -fsanitize=undefined
        yes   - same as "dbg"
],,enable_g=none)

if test "$enable_g" = "yes"; then
    enable_g=dbg
fi

if test "$enable_g" = "dbg" ; then
    PAC_APPEND_FLAG([-g],[FCFLAGS])
    PAC_APPEND_FLAG([-g],[WRAPPER_FCFLAGS])
elif test "$enable_g" = "asan" ; then
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[FCFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=address -fno-omit-frame-pointer],[WRAPPER_FCFLAGS])
    PAC_APPEND_FLAG([-fsanitize=address],[LDFLAGS])
elif test "$enable_g" = "ubsan" ; then
    PAC_APPEND_FLAG([-O1 -g -fsanitize=undefined -fno-omit-frame-pointer],[FCFLAGS])
    PAC_APPEND_FLAG([-O1 -g -fsanitize=undefined -fno-omit-frame-pointer],[WRAPPER_FCFLAGS])
    PAC_APPEND_FLAG([-fsanitize=undefined],[LDFLAGS])
fi

AC_ARG_ENABLE([two-level-namespace],
              [AS_HELP_STRING([--enable-two-level-namespace],
                              [(Darwin only) Do not use `-Wl,-flat_namespace` to
                               link libmpifort.dylib. MacOS uses two-level namespace
                               to compile dylibs by default. This may cause issues
                               not resolving MPI constants, such as MPI_IN_PLACE.
                               Thus, we use flat_namespace by default. Enable this
                               option to use two-level-namespace instead. ])],
              [],
              [enable_two_level_namespace=no])

FCLIB_LDFLAGS=
AC_SUBST([FCLIB_LDFLAGS])

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

        # On the other hand, the -flat-namespace option allows linker to
        # unify the same common symbols in different dylibs. It needs to be
        # added in linking stage for both the shared library and the final
        # executable file.

        # In Fortran programs, we implement global definitions such as MPI_BOTTOM
        # as common symbols. We check the address of the user input and translate it
        # to the C global definition if it is equal to the internal pre-stored address
        # in the Fortran binding layer. Without -flat-namespace, we may get different
        # addresses and thus treat the predefined constant as a normal buffer.

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
                FCLIB_LDFLAGS="-Wl,-flat_namespace"
            fi
        fi
        ]
    )
fi

# Determine characteristics of the Fortran 90 compiler
# Find a Fortran 90 compiler.  Sets FC
# Work around bug in autoconf that adds -g to FCFLAGS
saveFCFLAGS="$FCFLAGS"
PAC_PROG_FC_WORKS
FCFLAGS=$saveFCFLAGS
if test "$pac_cv_prog_fc_works" = no ; then
    AC_MSG_ERROR([Could not find a usable Fortran 90 compiler. Check the output of configure and consult the installation manuals])
fi

# assume requirement for f77 and f90 are always met at this point
enable_f77=yes
enable_f90=yes
AM_CONDITIONAL([BUILD_F77_BINDING], [test "$enable_f77" = "yes"])
AM_CONDITIONAL([BUILD_FC_BINDING], [test "$enable_f90" = "yes"])

# check requirement for f08
PAC_FC_2008_SUPPORT([enable_f08=yes],[enable_f08=no])
AM_CONDITIONAL([BUILD_F08_BINDING], [test "$enable_f08" = "yes"])

fortran_bindings=
AC_SUBST([fortran_bindings])

if test "$enable_f77" = "yes" ; then
    PAC_APPEND_CSV([mpif.h], [fortran_bindings])
fi

if test "$enable_f90" = "yes" ; then
    PAC_APPEND_CSV([use mpi], [fortran_bindings])
fi

if test "$enable_f08" = "yes" ; then
    PAC_APPEND_CSV([use mpi_f08], [fortran_bindings])
    AC_DEFINE(HAVE_F08_BINDING, 1, [Define to 1 if we have Fortran 2008 binding])
fi

AM_PROG_AR

LT_PREREQ([2.2.6])
LT_INIT()

AC_CONFIG_HEADERS([mpifort_config.h])

AC_ARG_VAR([MPIFCLIBNAME],[can be used to override the name of the MPI fortran library (default: "mpifort")])
if test -z "$MPIFCLIBNAME" ; then
    MPIFCLIBNAME="mpifort" ;
fi
export MPIFCLIBNAME
AC_SUBST(MPIFCLIBNAME)

# generate rpath-related macros, needed by mpifort
fc_shlib_conf=fc_shlib.conf
PAC_COMPILER_SHLIB_FLAGS([FC],[$fc_shlib_conf])
AC_SUBST_FILE([fc_shlib_conf])

PAC_CHECK_VISIBILITY
AC_SUBST(VISIBILITY_CFLAGS)

# check weak symbols, needed by f77 binding
# duplicated from mpich configure

# If build with MPICH, if paraent configure set NEEDSPLIB to yes, skip weak symbol check
if test "$NEEDSPLIB" != "yes" ; then
    PAC_PROG_C_WEAK_SYMBOLS(enable_weak_symbols=yes,enable_weak_symbols=no)
    if test $enable_weak_symbols = "yes" ; then
        AC_DEFINE(USE_WEAK_SYMBOLS,1,[Define if weak symbols should be used])
        NEEDSPLIB=no
        if test "$pac_cv_prog_c_weak_symbols" = "pragma weak" ; then
            PAC_PROG_C_MULTIPLE_WEAK_SYMBOLS(AC_DEFINE(HAVE_MULTIPLE_PRAGMA_WEAK,1,[Define if multiple weak symbols may be defined]))
        fi
    else
        NEEDSPLIB=yes
    fi
fi
AM_CONDITIONAL([BUILD_PROFILING_LIB],[test "$NEEDSPLIB" = "yes"])

AC_ARG_ENABLE(multi-aliases,
	AS_HELP_STRING([--enable-multi-aliases],
		[Multiple aliasing to support multiple fortran compilers (default)]),,
		enable_multi_aliases=yes)


# MPI_STATUS_SIZE is hardcoded and need to be synchronized with mpi.h
# FIXME: check MPI_F_STATUS_SIZE from mpi.h
MPI_STATUS_SIZE=5
AC_SUBST([MPI_STATUS_SIZE])

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

# ---------------------------------------------------------------------------
# Support for the language bindings: Fortran 77, Fortran 90
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

# Determine the extension for Fortran 90 files (it isn't always .f90)
PAC_FC_EXT
FCEXT=$ac_fc_srcext
AC_SUBST(FCEXT)

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

# ----------------------------------------------------------------------------
# Now test for Fortran compiler characteristics
# ----------------------------------------------------------------------------
if test "$enable_f77" = "yes" ; then
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
fi

dnl By modifying mpif.h to use ! for comments, it can work with many f90
dnl compilers without creating a separate version.
dnl Note that this is run AFTER the AC_OUTPUT actions
AC_CONFIG_COMMANDS([src/include/mpif.h],[if test "$enable_f77" = yes ; then
    if test "$has_exclaim" = "yes" ; then
        sed -e 's/^C/\!/g' mpif_h/mpif.h > mpif.h.tmp
        mv mpif.h.tmp mpif_h/mpif.h
    fi
    if test "$has_fort_real8" = "yes" ; then
        sed -e 's/DOUBLE PRECISION/REAL*8/g' mpif_h/mpif.h > mpif.h.tmp
        mv mpif.h.tmp mpif_h/mpif.h
    fi
fi],
enable_f77=$enable_f77
has_exclaim=$has_exclaim
has_fort_real8=$pac_cv_fort_real8
includebuild_dir=$includebuild_dir
libbuild_dir=$libbuild_dir
bashWorks=$bashWorks)

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
AC_CACHE_CHECK([whether real*2 is supported],pac_cv_fort_real2,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      real*2 a])],
    pac_cv_fort_real2=yes,
    pac_cv_fort_real2=no)])
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
AC_CACHE_CHECK([whether complex*4 is supported],pac_cv_fort_complex4,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      complex*4 a])],
    pac_cv_fort_complex4=yes,
    pac_cv_fort_complex4=no)])
AC_CACHE_CHECK([whether complex*8 is supported],pac_cv_fort_complex8,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      complex*8 a])],
    pac_cv_fort_complex8=yes,
    pac_cv_fort_complex8=no)])
AC_CACHE_CHECK([whether complex*16 is supported],pac_cv_fort_complex16,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      complex*16 a])],
    pac_cv_fort_complex16=yes,
    pac_cv_fort_complex16=no)])
AC_CACHE_CHECK([whether complex*32 is supported],pac_cv_fort_complex32,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      complex*32 a])],
    pac_cv_fort_complex32=yes,
    pac_cv_fort_complex32=no)])
AC_CACHE_CHECK([whether double precision is supported],pac_cv_fort_double_precision,[
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[      double precision a])],
    pac_cv_fort_double_precision=yes,
    pac_cv_fort_double_precision=no)])

if test -z "$pac_cv_f77_sizeof_integer" -o \
    "X$pac_cv_f77_sizeof_integer" = "X0" ; then
    AC_MSG_ERROR([Unable to configure with Fortran support because configure could not determine the size of a Fortran INTEGER.  Consider setting CROSS_F77_SIZEOF_INTEGER to the length in bytes of a Fortran INTEGER])
fi

AC_DEFINE_UNQUOTED([MPI_CHARACTER_SIZE], [$pac_cv_f77_sizeof_character], [Define the size of Fortran CHARACTER])
AC_DEFINE_UNQUOTED([MPI_LOGICAL_SIZE], [$pac_cv_f77_sizeof_logical], [Define the size of Fortran LOGICAL])
AC_DEFINE_UNQUOTED([MPI_INTEGER_SIZE], [$pac_cv_f77_sizeof_integer], [Define the size of Fortran INTEGER])
AC_DEFINE_UNQUOTED([MPI_REAL_SIZE], [$pac_cv_f77_sizeof_real], [Define the size of Fortran REAL])
AC_DEFINE_UNQUOTED([MPI_DOUBLE_PRECISION_SIZE], [$pac_cv_f77_sizeof_double_precision], [Define the size of Fortran DOUBLE PRECISION])

if test "$pac_cv_fort_double_precision" = "yes"; then
    AC_DEFINE([MPI_DOUBLE_PRECISION_SUPPORTED], [1], [Define if Fortran DOUBLE PRECISION is supported])
fi
if test "$pac_cv_fort_logical1" = "yes"; then
    AC_DEFINE([MPI_LOGICAL1_SUPPORTED], [1], [Define if Fortran LOGICAL*1 is supported])
fi
if test "$pac_cv_fort_logical2" = "yes"; then
    AC_DEFINE([MPI_LOGICAL2_SUPPORTED], [1], [Define if Fortran LOGICAL*2 is supported])
fi
if test "$pac_cv_fort_logical4" = "yes"; then
    AC_DEFINE([MPI_LOGICAL4_SUPPORTED], [1], [Define if Fortran LOGICAL*4 is supported])
fi
if test "$pac_cv_fort_logical8" = "yes"; then
    AC_DEFINE([MPI_LOGICAL8_SUPPORTED], [1], [Define if Fortran LOGICAL*8 is supported])
fi
if test "$pac_cv_fort_logical16" = "yes"; then
    AC_DEFINE([MPI_LOGICAL16_SUPPORTED], [1], [Define if Fortran LOGICAL*16 is supported])
fi
if test "$pac_cv_fort_integer1" = "yes"; then
    AC_DEFINE([MPI_INTEGER1_SUPPORTED], [1], [Define if Fortran INTEGER*1 is supported])
fi
if test "$pac_cv_fort_integer2" = "yes"; then
    AC_DEFINE([MPI_INTEGER2_SUPPORTED], [1], [Define if Fortran INTEGER*2 is supported])
fi
if test "$pac_cv_fort_integer4" = "yes"; then
    AC_DEFINE([MPI_INTEGER4_SUPPORTED], [1], [Define if Fortran INTEGER*4 is supported])
fi
if test "$pac_cv_fort_integer8" = "yes"; then
    AC_DEFINE([MPI_INTEGER8_SUPPORTED], [1], [Define if Fortran INTEGER*8 is supported])
fi
if test "$pac_cv_fort_integer16" = "yes"; then
    AC_DEFINE([MPI_INTEGER16_SUPPORTED], [1], [Define if Fortran INTEGER*16 is supported])
fi
if test "$pac_cv_fort_real2" = "yes"; then
    AC_DEFINE([MPI_REAL2_SUPPORTED], [1], [Define if Fortran REAL*2 is supported])
fi
if test "$pac_cv_fort_real4" = "yes"; then
    AC_DEFINE([MPI_REAL4_SUPPORTED], [1], [Define if Fortran REAL*4 is supported])
fi
if test "$pac_cv_fort_real8" = "yes"; then
    AC_DEFINE([MPI_REAL8_SUPPORTED], [1], [Define if Fortran REAL*8 is supported])
fi
if test "$pac_cv_fort_real16" = "yes"; then
    AC_DEFINE([MPI_REAL16_SUPPORTED], [1], [Define if Fortran REAL*16 is supported])
fi
if test "$pac_cv_fort_complex4" = "yes"; then
    AC_DEFINE([MPI_COMPLEX4_SUPPORTED], [1], [Define if Fortran COMPLEX*4 is supported])
fi
if test "$pac_cv_fort_complex8" = "yes"; then
    AC_DEFINE([MPI_COMPLEX8_SUPPORTED], [1], [Define if Fortran COMPLEX*8 is supported])
fi
if test "$pac_cv_fort_complex16" = "yes"; then
    AC_DEFINE([MPI_COMPLEX16_SUPPORTED], [1], [Define if Fortran COMPLEX*16 is supported])
fi
if test "$pac_cv_fort_complex32" = "yes"; then
    AC_DEFINE([MPI_COMPLEX32_SUPPORTED], [1], [Define if Fortran COMPLEX*32 is supported])
fi

AC_LANG_PUSH([C])
AC_CHECK_SIZEOF(int)
# for matching C types for F08 ISO_C_BINDING
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_LANG_POP([C])

# Provide the corresponding C types for MPI_INTEGER
AC_MSG_CHECKING([for C type matching Fortran integer])
if test "$pac_cv_f77_sizeof_integer" == "$ac_cv_sizeof_int" ; then
    AC_MSG_RESULT([yes])
    AC_DEFINE(HAVE_FINT_IS_INT,1,[Define if Fortran integer are the same size as C ints])
    MPI_FINT=int
else
    AC_MSG_RESULT([no])
    AC_MSG_WARN([Fortran integers and C ints are not the same size.  C programs that directly use MPI_Fint may be incorrect.])
    if test "$pac_cv_f77_sizeof_integer" == "$ac_cv_sizeof_short" ; then
        MPI_FINT=short
    elif test "$pac_cv_f77_sizeof_integer" == "$ac_cv_sizeof_long" ; then
        MPI_FINT=long
    elif test "$pac_cv_f77_sizeof_integer" == "$ac_cv_sizeof_long_long" ; then
        MPI_FINT="long long"
    else
        AC_MSG_ERROR([Failed to determine C type for Fortran Integer.])
    fi
fi

# Try and compute the values of .true. and .false. in Fortran
# This code has been removed because the Fortran binding code does
# not yet support it.
PAC_F77_LOGICALS_IN_C([$MPI_FINT])

# Get the INTEGER_KIND, ADDRESS_KIND and OFFSET_KIND if possible
# We also need to check the size of MPI_Aint vs MPI_Fint, and
# define AINT_LARGER_THAN_FINT if aint is larger (this
# affects code in MPI_Address)
if test "$MPI_SIZEOF_AINT" != "0" -a \
    "$MPI_SIZEOF_AINT" -gt "$pac_cv_f77_sizeof_integer" ; then
    AC_DEFINE(HAVE_AINT_LARGER_THAN_FINT,1,[Define if addresses are larger than Fortran integers])
fi
if test "$MPI_SIZEOF_AINT" != 0 -a \
    "$MPI_SIZEOF_AINT" != "$pac_cv_f77_sizeof_integer" ; then
    AC_DEFINE(HAVE_AINT_DIFFERENT_THAN_FINT,1,[Define if addresses are a different size than Fortran integers])
fi

dnl Using the {} around testsize helps the comments work correctly
PAC_PROG_FC_INT_KIND(INTEGER_KIND,${pac_cv_f77_sizeof_integer},$CROSS_F90_INTEGER_KIND)
PAC_PROG_FC_INT_KIND(ADDRESS_KIND,${MPI_SIZEOF_AINT},$CROSS_F90_ADDRESS_KIND)
PAC_PROG_FC_INT_KIND(OFFSET_KIND,${MPI_SIZEOF_OFFSET},$CROSS_F90_OFFSET_KIND)

if test "$INTEGER_KIND" = "-1" ; then
    # In our experience, this usually means that there is some
    # problem building and/or running the f90 program.  Fail
    # in this case rather than attempt to continue
    AC_MSG_ERROR([Unable to determine Fortran 90 KIND values for either address-sized integers or offset-sized integers.])
fi
#
# Some compilers won't allow a -1 kind (e.g., absoft).  In this case,
# use a fallback (sizeof(int) kind)
if test "$ADDRESS_KIND" = "-1" ; then
    ADDRESS_KIND=$INTEGER_KIND
fi
if test "$OFFSET_KIND" = "-1" ; then
    OFFSET_KIND=$INTEGER_KIND
fi

# assuming MPI_SIZEOF_COUNT is the max of MPI_SIZEOF_AINT and MPI_SIZEOF_OFFSET
if test $MPI_SIZEOF_COUNT = $MPI_SIZEOF_AINT; then
    COUNT_KIND=$ADDRESS_KIND
else
    COUNT_KIND=$OFFSET_KIND
fi

if test "$enable_f08" = "yes"; then
    PAC_PROG_FC_C_INT_TYPE([$MPI_SIZEOF_AINT], [F08_C_AINT])
    PAC_PROG_FC_C_INT_TYPE([$MPI_SIZEOF_OFFSET], [F08_C_OFFSET])
    PAC_PROG_FC_C_INT_TYPE([$MPI_SIZEOF_COUNT], [F08_C_COUNT])
    AC_SUBST(F08_C_AINT)
    AC_SUBST(F08_C_COUNT)
    AC_SUBST(F08_C_OFFSET)
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
AC_SUBST(COUNT_KIND)

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

# TODO: add check, and if needed, add to cmd_f77 the option "-DLLIMPORT type"


# Set defaults for these values so that the Makefile in src/bindings/f90
# is valid even if fc is not enabled (this is necessary for the
# distclean target)
MPIMODNAME=mpi
MPICONSTMODNAME=mpi_constants
MPISIZEOFMODNAME=mpi_sizeofs
MPIBASEMODNAME=mpi_base
PMPIBASEMODNAME=pmpi_base

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
    PMPIBASEMODNAME=PMPI_BASE
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
    PMPIBASEMODNAME=pmpi_base
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
AC_SUBST(PMPIBASEMODNAME)

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

if test "X$modincdir" = "X" ; then
  modincdir=$includedir
fi
export modincdir
AC_SUBST(modincdir)

# Python 3 is needed to generate Fortran bindings
PAC_CHECK_PYTHON

# we can optionally skip python generation if the bindings are pre-generated
skip_gen_bindings=no
if test -z "$PYTHON" ; then
    skip_gen_bindings=yes
    if test "$enable_f77" = "yes" && ! test -f mpif_h/mpif.h.in ; then
        skip_gen_bindings=no
    fi
    if test "$enable_f90" = "yes" && ! test -f use_mpi/mpi_base.f90 ; then
        skip_gen_bindings=no
    fi
    if test "$enable_f08" = "yes" && ! test -f use_mpi_f08/mpi_f08.f90 ; then
        skip_gen_bindings=no
    fi
    if test "$skip_gen_bindings" = "yes" ; then
        AC_MSG_NOTICE([Use pre-generated Fortran mpi binding. To prevent issues, install Python 3 and re-run configure.])
    else
        AC_MSG_ERROR([Python 3 is required to generate F90 bindings but not found!])
    fi
fi


if test "$skip_gen_bindings" = no ; then
    # locate the python scripts
    if test -e "$main_top_srcdir/maint/gen_binding_f77.py" ; then
        # when build FROM_MPICH, we don't require local copy of maint folder
        maintdir="$main_top_srcdir/maint"
    elif test -e "../../../maint/gen_binding_f77.py" ; then
        maintdir="../../../maint"
    elif test -e "maint/gen_binding_f77.py" ; then
        maintdir="maint"
    else
        AC_MSG_ERROR([Can't find gen_binding_f77.py])
    fi

    if test "$enable_f77" = "yes" ; then
        cmd_f77="$PYTHON $maintdir/gen_binding_f77.py"
        if test -n "$mpi_h" ; then
            cmd_f77="$cmd_f77 -mpi-h=$mpi_h"
        fi
        cmd_f77="$cmd_f77 -integer-kind=$INTEGER_KIND -address-kind=$ADDRESS_KIND -count-kind=$COUNT_KIND -offset-kind=$OFFSET_KIND"
        # if mpi_h is mpi_mpich.in, pass in those autoconf macro values
        if test -n "$MPI_MAX_PROCESSOR_NAME" ; then
            cmd_f77="$cmd_f77 -max-processor-name=$MPI_MAX_PROCESSOR_NAME -max-version-string=$MPI_MAX_LIBRARY_VERSION_STRING -max-error-string=$MPI_MAX_ERROR_STRING -bsend-overhead=$BSEND_OVERHEAD"
        fi
        AC_CONFIG_COMMANDS([gen_binding_f77], [$cmd_gen_binding_f77], [cmd_gen_binding_f77="$cmd_f77"])
        fi

        if test "$enable_f90" = "yes" ; then
        PAC_FC_CHECK_IGNORE_TKR
        PAC_FC_ISO_C_BINDING
        cmd_f90="$PYTHON $maintdir/gen_binding_f90.py"
# pass in type sizes for MPI_SIZEOF
        cmd_f90="$cmd_f90 -f-logical-size=$pac_cv_f77_sizeof_logical"
        if test "$pac_fc_ignore_tkr" != "no" ; then
            cmd_f90="$cmd_f90 -ignore-tkr=$pac_fc_ignore_tkr"
        fi
        if test "$pac_fc_iso_c_binding" = "no" ; then
            cmd_f90="$cmd_f90 -iso-c-binding=no"
        fi
        AC_CONFIG_COMMANDS([gen_binding_f90], [$cmd_gen_binding_f90], [cmd_gen_binding_f90="$cmd_f90"])
    fi

    if test "$enable_f08" = "yes" ; then
        PAC_FC_CHECK_REAL128
        PAC_FC_CHECK_IGNORE_TKR_D
        cmd_f08="$PYTHON $maintdir/gen_binding_f08.py"
        if test -n "$mpi_h" ; then
            cmd_f08="$cmd_f08 -mpi-h=$mpi_h"
        fi
        if test "$pac_fc_has_real128" = "no" ; then
            cmd_f08="$cmd_f08 -no-real128"
        fi
        if test "$pac_fc_ignore_tkr_d" != "no" ; then
            cmd_f08="$cmd_f08 -ignore-tkr-d=$pac_fc_ignore_tkr_d"
        fi
        if test "$enable_romio" = "no" ; then
            cmd_f08="$cmd_f08 -no-mpiio"
        fi
        cmd_f08="$cmd_f08 -fint-size=$pac_cv_f77_sizeof_integer -aint-size=$MPI_SIZEOF_AINT -count-size=$MPI_SIZEOF_COUNT -cint-size=$ac_cv_sizeof_int"
        AC_CONFIG_COMMANDS([gen_binding_f08], [$cmd_gen_binding_f08], [cmd_gen_binding_f08="$cmd_f08"])
    fi
fi

AC_SUBST(MPIFORT_NAME)
AC_SUBST(MPIF90_NAME)
AC_SUBST(MPIF77_NAME)
PAC_GET_EXENAME("mpifort", MPIFORT_NAME)
PAC_GET_EXENAME("mpif90", MPIF90_NAME)
PAC_GET_EXENAME("mpif77", MPIF77_NAME)

# MPICH_MPIx_FLAGS are used by mpifort and friends.  They are like
# WRAPPER flags, but these are provided by the user.
AC_SUBST(MPICH_MPIFORT_CPPFLAGS)
AC_SUBST(MPICH_MPIFORT_FCFLAGS)
AC_SUBST(MPICH_MPIFORT_LDFLAGS)
AC_SUBST(MPICH_MPIFORT_LIBS)

AC_SUBST(MPICH_MPIF77_CPPFLAGS)
AC_SUBST(MPICH_MPIF77_FCFLAGS)
AC_SUBST(MPICH_MPIF77_LDFLAGS)
AC_SUBST(MPICH_MPIF77_LIBS)

AM_CONDITIONAL([INSTALL_MPIF90],[true])
AM_CONDITIONAL([INSTALL_MPIF77],[true])

PAC_CHECK_BASH
AM_CONDITIONAL([BUILD_BASH_SCRIPTS], [test "x$pac_cv_bash_works" = xyes])

AC_CONFIG_FILES([Makefile \
                 env/mpifort.sh \
                 env/mpifort.bash \
                 localdefs])

if test "$enable_f77" = "yes" ; then
    AC_CONFIG_FILES([mpif_h/setbotf.f \
                     mpif_h/cmblk.c])
fi
if test "$enable_f08" = "yes" ; then
    AC_CONFIG_FILES([use_mpi_f08/mpi_c_interface_types.f90])
fi
AC_OUTPUT
