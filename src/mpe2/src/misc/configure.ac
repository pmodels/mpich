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
AC_CONFIG_HEADER(mpe_misc_conf.h)
dnl
echo "Configuring MPE Miscellaneous Library with $ac_configure_args"
dnl
dnl Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/mpe_misc.h ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/mpe_misc.h )
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
MPI_CC=${MPI_CC:-$CC}
MPI_CFLAGS=${MPI_CFLAGS}
MPI_LIBS=${MPI_LIBS}

MPE_LIBNAME=${MPE_LIBNAME:-mpe}
PROF_LIBNAME="$MPE_LIBNAME"
PROF_LIBS="-l$PROF_LIBNAME"
 
dnl Fixup for make
PAC_MAKE_IS_GNUMAKE
PAC_MAKE_IS_BSD44
PAC_MAKE_IS_OSF
PAC_MAKE_VPATH
AC_SUBST(MAKE)
 
dnl If we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/mpe_misc.h -a -z "$VPATH" ; then
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

AC_ARG_ENABLE( mpedbg,
[--enable-mpedbg                    - Enable the -mpedbg command-line argument
                                     (e.g., errors can start an xterm running
                                     a debugger).  Only works with some
                                     workstation systems when used with MPICH.
                                     The default is enable=yes],
enable_mpedbg=yes )
dnl
if test "$enable_mpedbg" = "yes" ; then
    MPEDBG=yes
else
    MPEDBG=no
fi
 
if test -z "$nonopt" ; then nonopt=NONE ; fi
AC_CANONICAL_HOST
case "$host" in
   *alpha*)
        CFLAGS="$CFLAGS -DDBX_IS_OSF"
        MPI_CFLAGS="$MPI_CFLAGS -DDBX_IS_OSF"
        host=alpha
        ;;
   *rs6000*)
        host=rs6000
        CFLAGS="$CFLAGS -DMPI_rs6000"
        MPI_CFLAGS="$MPI_CFLAGS -DMPI_rs6000"
        ;;
   *solaris*)
        host=solaris
        CFLAGS="$CFLAGS -DMPI_solaris"
        MPI_CFLAGS="$MPI_CFLAGS -DMPI_solaris"
        ;;
    *linux*)
        host=linux
        CFLAGS="$CFLAGS -DMPI_LINUX"
        MPI_CFLAGS="$MPI_CFLAGS -DMPI_LINUX"
        ;;
    *sun4*)
        host=sun4
        CFLAGS="$CFLAGS -DMPI_sun4"
        MPI_CFLAGS="$MPI_CFLAGS -DMPI_sun4"
        ;;
    mips-sgi-irix*)
        host=irix
        CFLAGS="$CFLAGS -DMPI_IRIX"
        MPI_CFLAGS="$MPI_CFLAGS -DMPI_IRIX"
        ;;
esac

dnl Set the various build directories from their mpe_ prefixed env variables.
dnl
dnl includebuild_dir is for all user header files
dnl libbuild_dir is used for building libraries in before they are installed.
dnl binbuild_dir is for the scripts/programs
dnl

rootbuild_dir=`pwd`
for dir in include lib bin ; do
    dirname=${dir}build_dir
    mpe_dirname=mpe_${dirname}
    eval dirvalue=\$"$dirname"
    eval $dirname=\$\{${mpe_dirname}\}
done

for dir in include lib bin ; do
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

dnl We need to check that this has worked.  The autoconf macro is broken
AC_PROG_CPP
if test "$CPP" = "/lib/cpp" -a ! -x /lib/cpp ; then
   AC_MSG_ERROR( [configure could not find a working C preprocessor] )
fi

DBX_NAME=""
AC_PATH_PROG(DBX_NAME,dbx)
dnl if test -z "$DBX_NAME" ; then
dnl    if test -n "$MPICH_WARNING_SEP" ; then echo '>'$MPICH_WARNING_SEP ; fi
dnl    echo "*# Cannot find dbx in your path.  You can put dbx in your path and"
dnl    echo "*# reconfigure or else you will not be able to run the debugger code"
dnl    echo "*# using the dbx debugger."
dnl    if test -n "$MPICH_WARNING_SEP" ; then echo '<'$MPICH_WARNING_SEP ; fi
dnl fi
#
GDB_NAME=""
AC_PATH_PROG(GDB_NAME,gdb)
dnl if test -z "$GDB_NAME" ; then
dnl     if test -n "$MPICH_WARNING_SEP" ; then echo '>'$MPICH_WARNING_SEP ; fi
dnl     echo "*# Cannot find gdb in your path.  You can put gdb in your path and"
dnl     echo "*# reconfigure or else you will not be able to run the debugger code"
dnl     echo "*# using the gdb debugger."
dnl     if test -n "$MPICH_WARNING_SEP" ; then echo '<'$MPICH_WARNING_SEP ; fi
dnl fi

AC_HEADER_STDC
AC_C_CONST

dnl  Checking for specific functions needed for the package.
AC_HAVE_FUNCS( sysinfo )
AC_CHECK_FUNC( uname,
               AC_DEFINE( HAVE_UNAME, 1,
                          [Define if uname availalble] )
               haveuname=1 )
AC_CHECK_FUNC( gethostbyname,
               AC_DEFINE( HAVE_GETHOSTBYNAME, 1,
                          [Define if gethostbyname available] )
               havegethostbyname=1 )

dnl  Checking Headers
AC_CHECK_HEADERS( unistd.h stdlib.h string.h netdb.h sys/systeminfo.h sys/types.h)

dnl
AC_MSG_CHECKING([for sighandler_t])
AC_TRY_COMPILE([#include <signal.h>],[sighandler_t *f;],
has_sighandler_t=yes,has_sighandler_t=no)
AC_MSG_RESULT($has_sighandler_t)
if test "$has_sighandler_t" = "yes" ; then
    AC_DEFINE( HAVE_SIGHANDLER_T, 1, [Define if sighandler_t available] )
fi

dnl kill() may have been missing in headerfile,
dnl e.g. gcc -ansi removes kill()'s prototype.
AC_CHECK_FUNCS(kill)
if test "$ac_cv_func_kill" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <signal.h>],kill)
fi

AC_SUBST(MPI_IMPL)
AC_SUBST(MPE_BUILD_FORTRAN2C)
AC_SUBST(MPEDBG)
AC_SUBST(CC)
AC_SUBST(CLINKER)
AC_SUBST(CFLAGS)
AC_SUBST(MPI_CC)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LIBS)
AC_SUBST(PROF_LIBNAME)

AC_OUTPUT( Makefile src/Makefile src/dbxerr.c )
