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
AC_CONFIG_HEADER(mpe_graphics_conf.h)
dnl
echo "Configuring MPE X Graphics Library with $ac_configure_args"
dnl
dnl Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/mpe_graphics.h ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/mpe_graphics.h )
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
ANIM_LIBNAME="a$MPE_LIBNAME"

PROF_LIBS="-l$PROF_LIBNAME"
ANIM_LIBS="-l$ANIM_LIBNAME $PROF_LIBS"
XGRAPH_LIBS="$PROF_LIBS"

 
dnl Fixup for make
PAC_MAKE_IS_GNUMAKE
PAC_MAKE_IS_BSD44
PAC_MAKE_IS_OSF
PAC_MAKE_VPATH
AC_SUBST(MAKE)
 
dnl If we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/mpe_graphics.h -a -z "$VPATH" ; then
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
 
AC_ARG_ENABLE( debugcheck,
[--enable-debugcheck                - Turn on/off the debugging & diagnostic
                                     checking code in MPE code.
                                     The default is enable=no.] )

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

AC_PROG_CC_C_O
# set CC_DOES_C_O to the value of \${ac_cv_prog_cc_${ac_cc}_c_o}
eval CC_DOES_C_O=\${ac_cv_prog_cc_${ac_cc}_c_o}
AC_SUBST(CC_DOES_C_O)

dnl  We need to check that this has worked.  The autoconf macro is broken
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

AC_PATH_X

dnl  AC_PATH_X set no_x=yes if user invokes configure with --without-x,
dnl  or x_includes/x_libraries is/are empty
if test -z "$no_x" ; then
    if test -n "$x_includes" ; then
        X_INC=-I$x_includes
    fi
    if test -n "$x_libraries" ; then
        X_LIB="-L$x_libraries -lX11"
        ANIM_LIBS="$ANIM_LIBS -L$x_libraries -lX11"
        XGRAPH_LIBS="$XGRAPH_LIBS -L$x_libraries -lX11"
    else
        X_LIB="-lX11"
        ANIM_LIBS="$ANIM_LIBS -lX11"
        XGRAPH_LIBS="$XGRAPH_LIBS -lX11"
    fi
else
    PAC_MSG_ERROR( $enable_softerror,
                   [Could not find appropriate X windows libraries.  MPE routines that make use of X11 graphics will NOT be built unless you put X windows libraries in your path and reconfigure!] )
fi

AC_HEADER_STDC
AC_C_CONST

AC_CHECK_SIZEOF(void *)
AC_MSG_CHECKING(for long pointer)
if test "$ac_cv_sizeof_void_p" -gt 4 ; then
    AC_DEFINE( POINTER_64_BITS, 1, [Define if pointers are 64 bits] )
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

dnl  Checking for specific functions needed for the package.
AC_HAVE_FUNCS(system)

AC_CHECK_HEADERS( unistd.h stdlib.h string.h )

dnl
if test "$MPE_BUILD_FORTRAN2C" = "yes" ; then

#   Determine MPI_Fint
    if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
        AC_MSG_CHECKING( [for MPI_Fint] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ MPI_Fint aa=1 ], ,
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        AC_MSG_RESULT($mpe_link_ok)
    else
        # MPICH2,MPICH has MPI_Fint
        mpe_link_ok=yes
    fi
    if test "$mpe_link_ok" = "yes" ; then
        MPI_FINT_TYPE=MPI_Fint
    else
        AC_MSG_RESULT([Assumed MPI_Fint to be int])
        MPI_FINT_TYPE=int
        AC_DEFINE( MPI_Fint, int, [Define MPI_Fint as int] )
    fi

#   Determmine if MPI_Comm_f2c() and MPI_Comm_c2f() are there
    if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
        AC_MSG_CHECKING( [for MPI_Comm_c2f() and MPI_Comm_f2c()] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, , [
                              MPI_Comm comm; ${MPI_FINT_TYPE} aa;
                              ], [
                              aa=MPI_Comm_c2f(comm) ;
                              comm=MPI_Comm_f2c(aa) ;
                              ], [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        AC_MSG_RESULT($mpe_link_ok)
    else
        mpe_link_ok=yes
    fi
    if test "$mpe_link_ok" = "yes" ; then
        AC_DEFINE( HAVE_MPI_COMM_F2C, 1,
                   [Define if MPI_Comm_f2c and c2f available] )
    fi
fi

if test ! -d contrib ; then mkdir contrib ; fi

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

AC_SUBST(X_INC)
AC_SUBST(X_LIB)
AC_SUBST(XGRAPH_LIBS)

AC_SUBST(PROF_LIBNAME)
AC_SUBST(ANIM_LIBNAME)
AC_SUBST(PROF_LIBS)
AC_SUBST(ANIM_LIBS)


AC_OUTPUT( Makefile src/Makefile \
           contrib/life/Makefile contrib/mandel/Makefile \
           contrib/mastermind/Makefile contrib/test/Makefile \
           etc/mpe_mpianim.conf etc/mpe_graphics.conf )
