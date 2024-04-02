##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

m4_include([version.m4])
AC_INIT([mpifort_abi],
        MPICH_VERSION_m4,
        [discuss@mpich.org],
        [mpifort_abi],
        [http://www.mpich.org/])

AC_CONFIG_AUX_DIR([confdb])
AC_CONFIG_MACRO_DIR([confdb])

AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability-recursive foreign 1.12.3 silent-rules subdir-objects])
AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AM_PROG_CC_C_O
PAC_CHECK_VISIBILITY
AC_SUBST(VISIBILITY_CFLAGS)

AM_PROG_AR

LT_INIT([])
# Non-verbose make by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

# suppress default "-g -O2" from AC_PROG_FC
: ${FCFLAGS=""}
AC_PROG_FC

F77=$FC
FFLAGS=$FCFLAGS
AC_PROG_F77

AC_DEFINE([MPI_ABI], 1, [Define to use MPI ABI header.])
AC_DEFINE([MPI_ABI_FORT], 1, [Define to include MPI ABI Fortran header.])

# MPI C ABI standard sizes
MPI_STATUS_SIZE=8
AC_SUBST([MPI_STATUS_SIZE])

MPI_SIZEOF_AINT=8
MPI_SIZEOF_OFFSET=8
MPI_SIZEOF_COUNT=8
AC_SUBST([MPI_SIZEOF_AINT])
AC_SUBST([MPI_SIZEOF_OFFSET])
AC_SUBST([MPI_SIZEOF_COUNT])

# mpi_f08 mapping to C types
get_c_int_type 8
MPI_AINT=$pac_retval
MPI_OFFSET=$pac_retval
MPI_COUNT=$pac_retval

enable_f77=yes
enable_f90=no
enable_f08=no
PAC_FC_2008_SUPPORT([:],[enable_f08=no])

modincdir=$includedir
AC_SUBST(modincdir)

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
AM_CONDITIONAL([BUILD_PROFILING_LIB],[test "$NEEDSPLIB" = "yes"])

PAC_CHECK_VISIBILITY
AC_SUBST(VISIBILITY_CFLAGS)
# disable visibility if building profiling library
if test "$NEEDSPLIB" = "yes" ; then
    VISIBILITY_CFLAGS=""
fi

if test "$enable_f77" ; then
    PAC_PROG_C_UNALIGNED_DOUBLES(,
[AC_MSG_WARN(Your C compiler $CC does not support unaligned accesses
to doubles.  This is required for interoperation with
Fortran (the Fortran standard requires it).
You may need to specify an additional argument to your C compiler to
force it to allow unaligned accesses.)])
fi

if test "$enable_f77" = yes ; then
    # Check if $MPI_DEFAULT_FOPTS is valid with $F77
    if test -n "$MPI_DEFAULT_FOPTS" ; then
        PAC_F77_CHECK_COMPILER_OPTION( [$MPI_DEFAULT_FOPTS], [
            FFLAGS="$FFLAGS $MPI_DEFAULT_FOPTS"
        ])
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

if test "$enable_f90" = "yes" -o "$enable_f08" = "yes"; then
    # determine shared library flags for FC
    fc_shlib_conf=confdb/fc_shlib.conf
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
fi

PAC_C_MULTI_ATTR_ALIAS
if test "$pac_c_multi_attr_alias" = "yes" ; then
    PAC_F2C_ATTR_ALIGNED_SIZE([1],[CMB_1INT_ALIGNMENT])
    AC_SUBST(CMB_1INT_ALIGNMENT)
    PAC_F2C_ATTR_ALIGNED_SIZE([$MPI_STATUS_SIZE],[CMB_STATUS_ALIGNMENT],[32])
    AC_SUBST(CMB_STATUS_ALIGNMENT)
    if test "X$CMB_1INT_ALIGNMENT" != "X" -a "X$CMB_STATUS_ALIGNMENT" != "X" ; then
        AC_DEFINE(HAVE_C_MULTI_ATTR_ALIAS, 1, [Define if multiple __attribute__((alias)) are supported])
    fi
fi

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

    # get C types for Fortran fixed-size REAL and INTEGER. If no equivalent C type, set
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
    pac_cv_f77_sizeof_real2=0
    pac_cv_f77_sizeof_real4=0
    pac_cv_f77_sizeof_real8=0
    pac_cv_f77_sizeof_real16=0

    AC_DEFINE(HAVE_NO_FORTRAN_MPI_TYPES_IN_C,1,[Define if the Fortran types are not available in C])
fi

if test "$pac_cv_f77_sizeof_integer8" = "0" ; then
    FORTRAN_MPI_OFFSET=INTEGER
else
    FORTRAN_MPI_OFFSET="INTEGER*8"
fi
export FORTRAN_MPI_OFFSET

if test "$enable_f90" = "yes"; then
    # Determine the precision and range of the standard Fortran types.  This
    # isn't quite enough for a full implementation of the Type_create_f90_xxx
    # routines, but will handle most programs.  We can extend this by trying to
    # find (through selected_real_kind and selected_int_kind) types with larger or
    # smaller precisions and/or ranges than the basic types.
    PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of reals],
                            [real aa],
                            [precision(aa), ",", range(aa)],
                            [FC_REAL_MODEL],
                            [$CROSS_F90_REAL_MODEL])
    #
    PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of double precision],
                            [double precision aa],
                            [precision(aa), ",", range(aa)],
                            [FC_DOUBLE_MODEL],
                            [$CROSS_F90_DOUBLE_MODEL])
    #
    PAC_FC_SIMPLE_NUMBER_MODEL([the range of integer],
                            [integer aa],
                            [range(aa)],
                            [FC_INTEGER_MODEL],
                            [$CROSS_F90_INTEGER_MODEL])

    # Try to find the available integer kinds by using selected_int_kind
    # This produces a table of range,kind
    PAC_FC_AVAIL_INTEGER_MODELS([FC_ALL_INTEGER_MODELS],
                                [$CROSS_F90_ALL_INTEGER_MODELS])
    #
    PAC_FC_INTEGER_MODEL_MAP([FC_INTEGER_MODEL_MAP],
                            [$CROSS_F90_INTEGER_MODEL_MAP])
else
    FC_REAL_MODEL="6, 37"
    FC_DOUBLE_MODEL="15, 307"
    FC_INTEGER_MODEL="4"
    FC_INTEGER_MODEL_MAP="{ 9, 4, 4 },"
    FC_ALL_INTEGER_MODELS="9, 4,"
fi
AC_SUBST(FC_REAL_MODEL)
AC_SUBST(FC_DOUBLE_MODEL)
AC_SUBST(FC_INTEGER_MODEL)
AC_SUBST(FC_ALL_INTEGER_MODELS)
AC_SUBST(FC_INTEGER_MODEL_MAP)    

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

AM_CONDITIONAL([BUILD_F77_BINDING],[test "$enable_f77" = "yes"])
AM_CONDITIONAL([BUILD_F90_BINDING],[test "$enable_f90" = "yes"])
AM_CONDITIONAL([BUILD_F08_BINDING],[test "$enable_f08" = "yes"])

# Python 3 is needed to generate Fortran bindings
PAC_CHECK_PYTHON

if test "$enable_f90" = "yes" ; then
    PAC_FC_CHECK_IGNORE_TKR
    PAC_FC_ISO_C_BINDING
    if test -z "$PYTHON" ; then
        if test -f use_mpi/mpi_base.f90 ; then
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
        if test "$pac_fc_iso_c_binding" = "no" ; then
            cmd_f90="$cmd_f90 -iso-c-binding=no"
        fi
        AC_CONFIG_COMMANDS([gen_binding_f90], [$cmd_gen_binding_f90], [cmd_gen_binding_f90="$cmd_f90"])
    fi
fi

if test "$enable_f08" = "yes" ; then
    PAC_FC_CHECK_REAL128
    if test -z "$PYTHON" ; then
        if test -f use_mpi_f08/mpi_f08.f90 ; then
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

AC_CONFIG_FILES([
    Makefile
])

if test "$enable_f77" = "yes" ; then
    AC_CONFIG_FILES([
        mpif_h/setbotf.f
        mpif_h/setbot.c
    ])
fi
if test "$enable_f08" = "yes" ; then
    AC_CONFIG_FILES([
        use_mpi_f08/mpi_c_interface_types.f90
    ])
fi

AC_OUTPUT
