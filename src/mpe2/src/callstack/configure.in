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
AC_CONFIG_HEADER( mpe_callstack_conf.h )
dnl
echo "Configuring MPE CallStack Library with $ac_configure_args"
dnl
dnl Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/mpe_callstack.h.in ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/mpe_callstack.h.in )
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

dnl if we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/mpe_callstack.h.in -a -z "$VPATH" ; then
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

dnl Set the various build directories from their mpe_ prefixed env variables.
dnl
dnl includebuild_dir is for all user header files
dnl libbuild_dir is used to build the libraries in before they are installed.
dnl binbuild_dir is for the scripts/programs
dnl sbinbuild_dir is for all system admin tools
dnl etcbuild_dir is for all system admin configuration settings
dnl
rootbuild_dir=`pwd`
for dir in include lib bin etc ; do
    dirname=${dir}build_dir
    mpe_dirname=mpe_${dirname}
    eval dirvalue=\$"$dirname"
    eval $dirname=\$\{${mpe_dirname}\}
done

for dir in include lib bin etc ; do
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

dnl We need to check that this has worked.  The autoconf macro is broken
AC_PROG_CPP
if test "$CPP" = "/lib/cpp" -a ! -x /lib/cpp ; then
   AC_MSG_ERROR( [configure could not find a working C preprocessor] )
fi

AC_HEADER_STDC

AC_C_CONST
AC_C_INLINE

AC_CHECK_HEADERS( stdio.h string.h unistd.h fcntl.h )

mpe_callstack_impl_type=""
mpe_callstack_ldflags=""
if test "X$mpe_callstack_impl_type" = "X" ; then
    dnl Check if backtrace() API is available
    AC_CHECK_HEADERS( [execinfo.h], [ 
        AC_MSG_CHECKING( [whether <execinfo.h> defines backtrace_symbols_fd()] )
        AC_COMPILE_IFELSE( [
            AC_LANG_PROGRAM( [
                #include <execinfo.h>
            ], [
changequote(<<, >>)dnl
                void* buffer[10];
changequote([, ])dnl
                int   depth;
                depth = backtrace( buffer, 10 );
                backtrace_symbols_fd( buffer, depth, 1 );
            ] )
        ], [
            AC_MSG_RESULT(yes)
            mpe_callstack_impl_type="#define HAVE_BACKTRACE 1"
            dnl Only gcc compiler suite knows -rdynamic, other compilers like
            dnl ifort does not know -rdynamic.  So explicit linker option,
            dnl -Wl,--export-dynamic will be more universal.
            AC_MSG_CHECKING( [if <execinfo.h> links with -Wl,--export-dynamic] )
            old_LDFLAGS="$LDFLAGS"
            LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
            AC_LINK_IFELSE( [
                AC_LANG_PROGRAM( [
                    #include <execinfo.h>
                ], [
changequote(<<, >>)dnl
                    void* buffer[10];
changequote([, ])dnl
                    int   depth;
                    depth = backtrace( buffer, 10 );
                    backtrace_symbols_fd( buffer, depth, 1 );
                ] )
            ], [
                AC_MSG_RESULT(yes)
                mpe_callstack_ldflags="-Wl,--export-dynamic"
            ], [
                AC_MSG_RESULT(no)
                LDFLAGS="$old_LDFLAGS"
            ] ) dnl Endof AC_LINK_IFELSE
        ], [
            AC_MSG_RESULT(no)
        ] ) dnl Endof AC_COMPILE_IFELSE
    ] ) dnl Endof AC_CHECK_HEADERS
fi

if test "X$mpe_callstack_impl_type" = "X" ; then
    dnl Check if printstack() API is available
    AC_CHECK_HEADERS( [ucontext.h], [
        AC_MSG_CHECKING( [whether <ucontext.h> defines printstack()] )
        AC_COMPILE_IFELSE( [
            AC_LANG_PROGRAM( [
                #include <ucontext.h>
            ], [
                printstack( 1 );
            ] )
        ], [
            AC_MSG_RESULT(yes)
            AC_MSG_CHECKING( [whether printstack() can be linked] )
            AC_LINK_IFELSE( [
                AC_LANG_PROGRAM( [
                    #include <ucontext.h>
                ], [
                    printstack( 1 );
                ] )
            ], [
                AC_MSG_RESULT(yes)
                mpe_callstack_impl_type="#define HAVE_PRINTSTACK 1"
            ], [
                AC_MSG_RESULT(no)
            ] ) dnl Endof AC_LINK_IFELSE
        ], [
            AC_MSG_RESULT(no)
        ] ) dnl Endof AC_COMPILE_IFELSE
    ] ) dnl Endof AC_CHECK_HEADERS
fi

if test "X$mpe_callstack_impl_type" = "X" ; then
    dnl Since no available callstack implementation, fall back to default.
    if test "$ac_cv_header_string_h" = "yes" ; then
        mpe_callstack_string_h="#define HAVE_STRING_H 1"
    fi
    if test "$ac_cv_header_unistd_h" = "yes" ; then
        mpe_callstack_unistd_h="#define HAVE_UNISTD_H 1"
    fi
fi

AC_SUBST(mpe_callstack_impl_type)
AC_SUBST(mpe_callstack_ldflags)
AC_SUBST(mpe_callstack_string_h)
AC_SUBST(mpe_callstack_unistd_h)

dnl fdopen may have been missing in headerfile,
dnl e.g. gcc -ansi remove fdopen's prototype
AC_CHECK_FUNCS(fdopen)
if test "$ac_cv_func_fdopen" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],fdopen)
fi

AC_SUBST(MPE_BUILD_FORTRAN2C)
AC_SUBST(CC)
AC_SUBST(CLINKER)
AC_SUBST(CFLAGS)
AC_SUBST(MPI_CC)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LIBS)
AC_SUBST(PROF_LIBNAME)

AC_OUTPUT_COMMANDS( [
if test ! -x ./bin/bt2line -a -s ./bin/bt2line ; then
    chmod a+x ./bin/bt2line
fi
] )
AC_OUTPUT( Makefile src/Makefile include/mpe_callstack.h \
           etc/mpe_callstack_ldflags.conf bin/bt2line )
