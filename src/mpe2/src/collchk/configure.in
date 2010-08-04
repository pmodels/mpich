dnl
dnl Caching is usually WRONG for systems with cross-mounted file systems
dnl (the cache file may correspond to a different system).  Since configure
dnl is not on a performance-critical path, go for robustness over speed.
dnl
define([AC_CACHE_LOAD], )dnl
define([AC_CACHE_SAVE], )dnl
dnl
dnl
AC_INIT()
AC_PREREQ(2.52)
AC_CONFIG_HEADER(mpe_collchk_conf.h)
dnl
echo "Configuring MPE Collective and Datatype Checking Library with $ac_configure_args"
dnl
dnl Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/collchk.h ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/collchk.h )
fi
AC_SUBST(top_srcdir_abs)

AC_ARG_ENABLE( echo,
[--enable-echo                      - Turn on strong echoing.
                                     The default is enable=no.],
set -x )

AC_ARG_ENABLE( softerror,
[--enable-softerror                 - Turn on soft error, i.e. instead of
                                     issuing MSG_ERROR, MSG_WARN + exit 0
                                     are used.  The default is enable=no.],
, enable_softerror=${mpe_enable_softerror:-no} )

MAKE=${MAKE:-make}
MPI_IMPL=${MPI_IMPL:-UserSupplied}
MPE_BUILD_FORTRAN2C=${MPE_BUILD_FORTRAN2C:-no}
CC=${CC:-cc}
CLINKER=${CLINKER:-$CC}
CFLAGS=${MPE_CFLAGS}
MPI_CC=${MPI_CC:-${CC}}
MPE_CLINKER=${MPE_CLINKER}
F77=${F77:-f77}
FLINKER=${FLINKER:-$F77}
FFLAGS=${MPE_FFLAGS}
MPI_F77=${MPI_F77:-${F77}}
MPE_FLINKER=${MPE_FLINKER}
FINC_PATH_LEADER=${FINC_PATH_LEADER:-"-I"}
FLIB_PATH_LEADER=${FLIB_PATH_LEADER:-"-L"}
MPI_CFLAGS=${MPI_CFLAGS}
MPI_FFLAGS=${MPI_FFLAGS}
MPI_LIBS=${MPI_LIBS}

MPE_LIBNAME=${MPE_LIBNAME:-mpe}
PROF_LIBNAME="$MPE_LIBNAME"
COLLCHK_LIBNAME="${MPE_LIBNAME}_collchk"
F2CMPI_LIBNAME="${MPE_LIBNAME}_f2cmpi"

PROF_LIBS="-l$PROF_LIBNAME"
COLLCHK_LIBS="-l$COLLCHK_LIBNAME $PROF_LIBS"
F2CMPI_LIBS="-l$F2CMPI_LIBNAME"

dnl Fixup for make
PAC_MAKE_IS_GNUMAKE
PAC_MAKE_IS_BSD44
PAC_MAKE_IS_OSF
PAC_MAKE_VPATH
AC_SUBST(MAKE)

dnl If we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/collchk.h -a -z "$VPATH" ; then
    AC_MSG_ERROR( [No virtual MAKE path command found.
        You may need to set your make command
        The GNU make (sometimes available as gnumake) can be used.] )
fi

dnl CPRP is the version of cp that accepts -r and -p arguments.
MKDIR=mkdir
CPRP=cp
RM=rm
MV=mv
AC_SUBST(MKDIR)
AC_SUBST(CPRP)
AC_SUBST(RM)
AC_SUBST(MV)

AC_ARG_WITH( mpiio,
[--with-mpiio                       - Enable/Disable MPI-IO checking.
                                     in MPICHx buiid. The default is yes.], ,
with_mpiio=yes )

AC_ARG_ENABLE( romio,
[--enable-romio                     - Enable/Disable ROMIO MPI-IO checking
                                     in MPICHx build. The default is yes.] ,
with_mpiio=$enableval,
with_mpiio=yes )

AC_ARG_ENABLE( checkMPI,
[--enable-checkMPI                  - Turn on/off the linking test for the MPI
                                     include files, MPI libraries and MPI_CC
                                     compiler. The default is enable=yes.], ,
enable_checkMPI=yes )

AC_ARG_WITH( checkMPIO,
[--enable-checkMPIO                 - Turn on/off the linking test for MPI-IO
                                     routines in MPI implementation.
                                     The default is enable=yes.], ,
enable_checkMPIO=yes )

AC_ARG_WITH( f2cmpilibs,
[--with-f2cmpilibs=F2CMPI_LIBS      - compiler's library flags for
                                     Fortran to C MPI wrapper library.
                                     Using this option disables building of
                                     MPE's internal Fortran to C MPI library,
                                     --disable-f2cmpilib.  The library is
                                     needed for profiling MPI Fortran program.
                                     e.g. "-lfmpich" when MPE is building with
                                     MPICH.],
F2CMPI_LIBS="$withval" )

if test -z "$nonopt" ; then nonopt=NONE ; fi
AC_CANONICAL_HOST

dnl Set the various build directories from their mpe_ prefixed env variables.
dnl
dnl includebuild_dir is for all user header files
dnl libbuild_dir is used to build the libraries in before they are installed.
dnl binbuild_dir is for the scripts/programs
dnl sbinbuild_dir is for all system admin tools
dnl etcbuild_dir is for all system admin configuration settings
dnl
rootbuild_dir=`pwd`
for dir in include lib bin sbin etc ; do
    dirname=${dir}build_dir
    mpe_dirname=mpe_${dirname}
    eval dirvalue=\$"$dirname"
    eval $dirname=\$\{${mpe_dirname}\}
done

for dir in include lib bin sbin etc ; do
    dirname=${dir}build_dir
    eval dirvalue=\$"$dirname"
    if test -n "$dirvalue" ; then
        if test ! -d $dirvalue ; then
            if mkdir -p $dirvalue ; then
                :
            else
                PAC_MSG_ERROR( $enable_softerror,
                               [Could not create directory $dirvalue] )
            fi
        fi
    fi
done

AC_SUBST(includebuild_dir)
AC_SUBST(libbuild_dir)
AC_SUBST(binbuild_dir)
AC_SUBST(sbinbuild_dir)
AC_SUBST(etcbuild_dir)

AC_PROG_CC
AC_OBJEXT
AC_EXEEXT
dnl Check for broken handling of common symbols
dnl PAC_PROG_C_BROKEN_COMMON
AC_CHECK_PROG(AR, ar, ar, ;)
AC_PROG_RANLIB

dnl We need to check that this has worked.  The autoconf macro is broken
AC_PROG_CPP
if test "$CPP" = "/lib/cpp" -a ! -x /lib/cpp ; then
   AC_MSG_ERROR( [configure could not find a working C preprocessor] )
fi

dnl  Invoking AC_CHECK_LIB before AC_PROG_CC makes AC_PROG_CC fail
dnl  in autoconf 2.59
DEBUG_LIBS=""
if test "$enable_debugcheck" = "yes" ; then
    AC_CHECK_LIB( efence, malloc, DEBUG_LIBS="-lefence" )
fi
AC_SUBST(DEBUG_LIBS)

AC_HEADER_STDC
AC_C_CONST

dnl  Checking Headers
AC_CHECK_HEADERS( stdio.h stdlib.h string.h unistd.h )

AC_FUNC_ALLOCA

dnl Check MPI_Type_get_envelope() and MPI_Type_get_content()
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_Datatype decoding functions] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [
    int           nints, naddrs, ntypes, combiner;
    int          *ints; 
    MPI_Aint     *addrs;
    MPI_Datatype *types;
    MPI_Datatype  type;
    int           itmp
                              ],
                              [
    itmp=MPI_Type_get_envelope( type, &nints, &naddrs, &ntypes, &combiner );
    itmp=MPI_Type_get_contents(type, nints, naddrs, ntypes, ints, addrs, types);
    combiner=MPI_COMBINER_NAMED;
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
        else
            AC_MSG_RESULT(no)
            PAC_MSG_ERROR( $enable_softerror,
                           [Missing MPI_Datatype decoding functions!] )
        fi
    fi
dnl if MPICH2/MPICH1, assume MPI_Type_get_envelope()/MPI_Type_get_content()
fi

dnl Check MPI_COMBINER_xxx
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for rare MPI_COMBINER_XXX types] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ int cbr1, cbr2, cbr3, cbr4, cbr5, cbr6; ],
                              [
    cbr1=MPI_COMBINER_DUP; cbr2=MPI_COMBINER_F90_COMPLEX;
    cbr3=MPI_COMBINER_INDEXED_BLOCK; cbr4=MPI_COMBINER_HINDEXED_INTEGER;
    cbr5=MPI_COMBINER_STRUCT_INTEGER; cbr6=MPI_COMBINER_SUBARRAY;
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_RARE_MPI_COMBINERS, 1,
                       [Define if rare MPI_COMBINER_xxx exist] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume are MPI_COMBINER_XXX are available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_RARE_MPI_COMBINERS, 1,
                   [Define if rare MPI_COMBINER_xxx exist] )
    fi
fi


dnl Check MPI_Add_error_class(), MPI_Add_error_code(), MPI_Add_error_string()
dnl and MPI_Comm_call_errhandler()
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI Error functions] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [
    int itmp, err_class, err_code;
    char err_str[]="test";
                              ],
                              [
    itmp=MPI_Add_error_class( &err_class );
    itmp=MPI_Add_error_code( err_class, &err_code );
    itmp=MPI_Add_error_string( err_code, err_str );
    itmp=MPI_Comm_call_errhandler( MPI_COMM_WORLD, err_code );
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_ERR_FNS, 1,
                       [Define if MPI Error functions exist] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume MPI_IN_PLACE is available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_MPI_ERR_FNS, 1, [Define if MPI Error functions exist] )
    fi
fi


dnl Check MPI_IN_PLACE
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_IN_PLACE] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ int itmp, iboolean; ],
                              [
    itmp=MPI_Allreduce( MPI_IN_PLACE, &iboolean, 1, MPI_INT,
                        MPI_LAND, MPI_COMM_WORLD )
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_IN_PLACE, 1, [Define if MPI_IN_PLACE exists] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume MPI_IN_PLACE is available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_MPI_IN_PLACE, 1, [Define if MPI_IN_PLACE exists] )
    fi
fi

dnl Check MPI_WCHAR
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_WCHAR] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Datatype type; ],
                              [ type = MPI_WCHAR ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_WCHAR, 1,
                       [Define if MPI_WCHAR exists] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume MPI_SIGNED_CHAR is available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_MPI_WCHAR, 1,
                   [Define if MPI_WCHAR exists] )
    fi
fi

dnl Check MPI_SIGNED_CHAR
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_SIGNED_CHAR] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Datatype type; ],
                              [ type = MPI_SIGNED_CHAR ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_SIGNED_CHAR, 1,
                       [Define if MPI_SIGNED_CHAR exists] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume MPI_SIGNED_CHAR is available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_MPI_SIGNED_CHAR, 1,
                   [Define if MPI_SIGNED_CHAR exists] )
    fi
fi

dnl Check MPI_UNSIGNED_LONG_LONG
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_UNSIGNED_LONG_LONG] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Datatype type; ],
                              [ type = MPI_UNSIGNED_LONG_LONG ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_UNSIGNED_LONG_LONG, 1,
                       [Define if MPI_UNSIGNED_LONG_LONG exists] )
        else
            AC_MSG_RESULT(no)
        fi
    fi
else
dnl if MPICH2, assume MPI_SIGNED_CHAR is available, not in MPICH1
    if test "$MPI_IMPL" = "MPICH2" ; then
        AC_DEFINE( HAVE_MPI_UNSIGNED_LONG_LONG, 1,
                   [Define if MPI_UNSIGNED_LONG_LONG exists] )
    fi
fi

dnl Check Fortran MPI_Datatype definitions in C
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for Fortran MPI_Datatype in C] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Datatype type ],
                              [
    type = MPI_COMPLEX;
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_IN_C, 1,
                       [Define if Fortran MPI_Datatype exists in C] )

dnl         Check for MPI_INTEGERx
            AC_MSG_CHECKING( [for Fortran MPI_Datatype, INTEGER related, in C] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Datatype type1, type2, type3, type4 ],
                                  [
    type1 = MPI_INTEGER1;
    type2 = MPI_INTEGER2;
    type3 = MPI_INTEGER4;
    type4 = MPI_INTEGER8;
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_INTEGERX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, INTEGERs, exist in C] )
            else
                AC_MSG_RESULT(no)
            fi

dnl         Check for MPI_REALx
            AC_MSG_CHECKING( [for Fortran MPI_Datatype, REAL related, in C] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Datatype type1, type2 ],
                                  [
    type1 = MPI_REAL4;
    type2 = MPI_REAL8;
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_REALX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, REALs, exist in C] )
            else
                AC_MSG_RESULT(no)
            fi

dnl         Check for MPI_COMPLEXx 
            AC_MSG_CHECKING( [for Fortran MPI_Datatype, COMPLEXn related, in C] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Datatype type1, type2 ],
                                  [
    type1 = MPI_COMPLEX8;
    type2 = MPI_COMPLEX16;
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_COMPLEXX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, COMPLEXs, exist in C] )
            else
                AC_MSG_RESULT(no)
            fi

dnl         Check for MPI_2COMPLEXxxx
            AC_MSG_CHECKING( [for Fortran MPI_Datatype, 2COMPLEX related, in C] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Datatype type1, type2 ],
                                  [
    type1 = MPI_2COMPLEX;
    type2 = MPI_2DOUBLE_COMPLEX;
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_2COMPLEX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, 2COMPLEX, exist in C] )
            else
                AC_MSG_RESULT(no)
            fi

        else

dnl         if HAVE_FORTRAN_MPI_DATATYPE_IN_C is false
            AC_MSG_RESULT(no)
            AC_MSG_CHECKING( [for LAM/MPI Fortran MPI_Datatype in C] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Datatype type ],
                                  [
#define MPI_COMPLEX   ((MPI_Datatype) &lam_mpi_cplex)
    type = MPI_COMPLEX;
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_LAM_FORTRAN_MPI_DATATYPE_IN_C, 1,
                           [Define if LAM Fortran MPI_Datatype exists in C] )
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_IN_C, 1,
                           [Define if Fortran MPI_Datatype exists in C] )
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_INTEGERX_IN_C, 1,
                        [Define if Fortran MPI_Datatype, INTEGERs, exist in C] )
                AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_REALX_IN_C, 1,
                           [Define if Fortran MPI_Datatype, REALs, exist in C] )
            else
                AC_MSG_RESULT(no)
            fi
        fi
    fi
else
dnl if MPICH2, assume all Fortran MPI_Datatypes are available in C
dnl if MPICH1, assume only basic Fortran MPI_Datatypes are available in C
dnl if MPICH2, assume 2COMPLEX is *not* available (default as of 1.3x)
    if test "$MPI_IMPL" = "MPICH2" ; then
         AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_IN_C, 1,
                    [Define if Fortran MPI_Datatype exists in C] )
         AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_INTEGERX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, INTEGERs, exist in C] )
         AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_REALX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, REALs, exist in C] )
         AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_COMPLEXX_IN_C, 1,
                    [Define if Fortran MPI_Datatype, COMPLEXs, exist in C] )
    else
         AC_DEFINE( HAVE_FORTRAN_MPI_DATATYPE_IN_C, 1,
                    [Define if Fortran MPI_Datatype exists in C] )
    fi
fi

dnl Check if MPI_Datatype is pointer or int
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for MPI_Datatype] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Datatype type ],
                              [ type = type / 5 ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT([int])
            AC_DEFINE( HAVE_INT_MPI_DATATYPE, 1,
                       [Define if MPI_Datatype is int] )
        else
            AC_MSG_RESULT([void*])
            AC_DEFINE( HAVE_PTR_MPI_DATATYPE, 1,
                       [Define if MPI_Datatype is void*] )
        fi
    fi
else
dnl if MPICH2 or MPICH1, assume MPI_Datatype is int
    AC_DEFINE( HAVE_INT_MPI_DATATYPE, 1,
               [Define if MPI_Datatype is int] )
fi


dnl Check MPI_Exscan
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_Exscan] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ int itmp, ibuff ],
                              [
    itmp=MPI_Exscan( &ibuff, &ibuff, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD )
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [for the C version of PMPI_Exscan] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ int itmp, ibuff ],
                                  [
    itmp=PMPI_Exscan( &ibuff, &ibuff, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD )
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                COLLCHK_BUILD_EXSCAN=yes
            else
                AC_MSG_RESULT(no)
                COLLCHK_BUILD_EXSCAN=no
            fi
        else
            AC_MSG_RESULT(no)
            COLLCHK_BUILD_EXSCAN=no
        fi
    fi
else
dnl if MPICH2, assume MPI_Exscan is available.
    if test "$MPI_IMPL" = "MPICH2" ; then
        COLLCHK_BUILD_EXSCAN=yes
    else
        COLLCHK_BUILD_EXSCAN=no
    fi
fi

dnl Check MPI_Alltoallw
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_Alltoallw] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [[
#define MAX 10
    int *sbuff, *rbuff;
    int scounts[MAX], rcounts[MAX], sdispls[MAX], rdispls[MAX];
    MPI_Datatype  stypes[MAX], rtypes[MAX];
    int itmp
                              ]],
                              [
    itmp=MPI_Alltoallw( sbuff, scounts, sdispls, stypes,
                        rbuff, rcounts, rdispls, rtypes, MPI_COMM_WORLD )
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [for the C version of PMPI_Alltoallw] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [[
#define MAX 10
    int *sbuff, *rbuff;
    int scounts[MAX], rcounts[MAX], sdispls[MAX], rdispls[MAX];
    MPI_Datatype  stypes[MAX], rtypes[MAX];
    int itmp
                                  ]],
                                  [
    itmp=PMPI_Alltoallw( sbuff, scounts, sdispls, stypes,
                         rbuff, rcounts, rdispls, rtypes, MPI_COMM_WORLD )
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                COLLCHK_BUILD_ALLW=yes
            else
                AC_MSG_RESULT(no)
                COLLCHK_BUILD_ALLW=no
            fi
        else
            AC_MSG_RESULT(no)
            COLLCHK_BUILD_ALLW=no
        fi
    fi
else
dnl if MPICH2, assume MPI_Exscan is available.
    if test "$MPI_IMPL" = "MPICH2" ; then
        COLLCHK_BUILD_ALLW=yes
    else
        COLLCHK_BUILD_ALLW=no
    fi
fi

dnl Check MPI-IO
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" -a "$enable_checkMPIO" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_File_open()] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ int itmp; MPI_File fh ],
                              [ itmp=MPI_File_open( MPI_COMM_WORLD, "mpiiotest", MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh ) ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [for the C version of PMPI_File_open()] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ int itmp; MPI_File fh ],
                                  [ itmp=PMPI_File_open( MPI_COMM_WORLD, "mpiiotest", MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh ) ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_MSG_CHECKING( [for ROMIO's MPIO_Request] )
                PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                      [ MPIO_Request  io_request], ,
                                      [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
                if test "$mpe_link_ok" = "yes" ; then
                    AC_MSG_RESULT(yes)
                    COLLCHK_BUILD_IO=yes
                    AC_DEFINE( HAVE_MPI_IO, 1,
                               [Define if MPI-IO routines exist.] )
                else
                    AC_MSG_RESULT(no)
                    COLLCHK_BUILD_IO=no
                fi
            else
                AC_MSG_RESULT([no, assumed No MPI-IO routines])
                COLLCHK_BUILD_IO=no
            fi
        else
            AC_MSG_RESULT([no, assumed No MPI-IO routines])
            COLLCHK_BUILD_IO=no
        fi
    fi
else
dnl When built with MPICHx, MPE configure observes --with-mpiio & --enable-romio
dnl if test \( "$MPI_IMPL" = "MPICH" -o "$MPI_IMPL" = "MPICH2" \) \
    if test "$with_mpiio" = "yes" ; then
        COLLCHK_BUILD_IO=yes
        AC_DEFINE( HAVE_MPI_IO, 1, [Define if MPI-IO routines exist.] )
    else
        COLLCHK_BUILD_IO=no
    fi
fi

dnl Check MPI-RMA
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_Win_create] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ int itmp; MPI_Win win ],
                              [
    itmp=MPI_Win_create( NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win )
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [for the C version of PMPI_Win_create] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ int itmp; MPI_Win win ],
                                  [
    itmp=PMPI_Win_create( NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win )
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                COLLCHK_BUILD_RMA=yes
                AC_DEFINE( HAVE_MPI_RMA, 1,
                           [Define if MPI-RMA routines exist.] )
            else
                AC_MSG_RESULT([no, assumed No MPI-RMA routines])
                COLLCHK_BUILD_RMA=no
            fi
        else
            AC_MSG_RESULT([no, assumed No MPI-RMA routines])
            COLLCHK_BUILD_RMA=no
        fi
    fi
else
dnl if MPICH2, assume MPI-RMA is available.
    if test "$MPI_IMPL" = "MPICH2" ; then
        COLLCHK_BUILD_RMA=yes
        AC_DEFINE( HAVE_MPI_RMA, 1, [Define if MPI-RMA routines exist.] )
    else
        COLLCHK_BUILD_RMA=no
    fi
fi

dnl Check MPI-COMMS
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_Comm_spawn] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Comm intercomm;
                                int itmp, np=1, ierrs; ],
                              [
    itmp=MPI_Comm_spawn( "./test", MPI_ARGV_NULL, np, MPI_INFO_NULL, 0,
                         MPI_COMM_WORLD, &intercomm, &ierrs )
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [for the C version of PMPI_Comm_spawn] )
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                                  [ MPI_Comm intercomm;
                                    int itmp, np=1, ierrs; ],
                                  [
    itmp=PMPI_Comm_spawn( "./test", MPI_ARGV_NULL, np, MPI_INFO_NULL, 0,
                          MPI_COMM_WORLD, &intercomm, &ierrs )
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                COLLCHK_BUILD_COMMS=yes
                AC_DEFINE( HAVE_MPI_COMMS, 1,
                           [Define if MPI-RMA routines exist.] )
            else
                AC_MSG_RESULT([no, assumed No MPI-COMMS routines])
                COLLCHK_BUILD_COMMS=no
            fi
        else
            AC_MSG_RESULT([no, assumed No MPI-COMMS routines])
            COLLCHK_BUILD_COMMS=no
        fi
    fi
else
dnl if MPICH2, assume MPI_COMMS is available.
    if test "$MPI_IMPL" = "MPICH2" ; then
        COLLCHK_BUILD_COMMS=yes
        AC_DEFINE( HAVE_MPI_COMMS, 1, [Define if MPI-RMA routines exist.] )
    else
        COLLCHK_BUILD_COMMS=no
    fi
fi

AC_SUBST(COLLCHK_BUILD_EXSCAN)
AC_SUBST(COLLCHK_BUILD_ALLW)
AC_SUBST(COLLCHK_BUILD_IO)
AC_SUBST(COLLCHK_BUILD_RMA)
AC_SUBST(COLLCHK_BUILD_COMMS)

AC_SUBST(MPI_IMPL)
AC_SUBST(MPE_BUILD_FORTRAN2C)
AC_SUBST(CC)
AC_SUBST(CLINKER)
AC_SUBST(CFLAGS)
AC_SUBST(MPI_CC)
AC_SUBST(MPE_CLINKER)
AC_SUBST(F77)
AC_SUBST(FLINKER)
AC_SUBST(FFLAGS)
AC_SUBST(MPI_F77)
AC_SUBST(MPE_FLINKER)
AC_SUBST(FINC_PATH_LEADER)
AC_SUBST(FLIB_PATH_LEADER)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_FFLAGS)
AC_SUBST(MPI_LIBS)

AC_SUBST(COLLCHK_LIBNAME)
AC_SUBST(COLLCHK_LIBS)
AC_SUBST(F2CMPI_LIBS)

AC_OUTPUT( Makefile src/Makefile test/Makefile \
           etc/mpe_mpicheck.conf )
