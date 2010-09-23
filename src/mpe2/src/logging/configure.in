dnl
dnl Caching is usually WRONG for systems with cross-mounted file systems
dnl (the cache file may correspond to a different system).  Since configure
dnl is not on a performance-critical path, go for robustness over speed.
dnl
define([AC_CACHE_LOAD], )dnl
define([AC_CACHE_SAVE], )dnl
dnl
dnl
AH_BOTTOM([
/* Temporary fix of undefined int64_t on SLES8 with "icc -no-gcc" */
#if defined(FIX_STDINT_ORDER)
#include "clog_inttypes.h"
#endif

/* Define WINDOWS specific features */
/*
   Windows' open() opens an ASCII file by default, add Windows specific
   flag O_BINARY to open()'s argument
*/
#if !defined( OPEN )

#if defined(HAVE_WINDOWS_H)

#define OPEN( a, b, c )    open( a, b | O_BINARY, c )

#else

#if defined(HAVE_O_LARGEFILE)

#if defined(HAVE_O_BINARY)
#define OPEN( a, b, c )    open( a, b | O_LARGEFILE | O_BINARY, c )
#else
#define OPEN( a, b, c )    open( a, b | O_LARGEFILE, c )
#endif    /* HAVE_O_BINARY */

#else

#if defined(HAVE_O_BINARY)
#define OPEN( a, b, c )    open( a, b | O_BINARY, c )
#else
#define OPEN( a, b, c )    open( a, b, c )
#endif    /* HAVE_O_BINARY */

#endif    /* HAVE_O_LARGEFILE */

#endif    /* HAVE_WINDOWS_H */

#endif    /* OPEN */
])
AC_INIT()
AC_PREREQ(2.52)
AC_CONFIG_HEADER( mpe_logging_conf.h )
dnl
echo "Configuring MPE Logging Library and Tools with $ac_configure_args"
dnl
dnl Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/mpe_log.h ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/mpe_log.h )
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
 
dnl If we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/mpe_log.h -a -z "$VPATH" ; then
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

AC_ARG_ENABLE( threadlogging,
[--enable-threadlogging             - Turn on/off the coarse-grained thread
                                      support in MPE logging.  The default is
                                      enable=yes.], ,
enable_threadlogging=yes )

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

AC_DEFINE( CLOG_IMPL, 1,
           [The configuration header enables CLOG2 implementation related API] )

dnl   -- Check if byteswapping needs to be done.
dnl if test "$cross_compiling" = "yes" -o "$cross_compiling" = 1 ; then
dnl     AC_MSG_CHECKING([for byte ordering])
dnl     if test "X$CROSS_BIGENDIAN" != "X" ; then
dnl         if test "$CROSS_BIGENDIAN" = "true" \
dnl              -o "$CROSS_BIGENDIAN" = "false" ; then
dnl             if test "$CROSS_BIGENDIAN" = "true" ; then
dnl                 AC_DEFINE( WORDS_BIGENDIAN, 1,
dnl                            [Define if byte order is bigendian] )
dnl                 AC_MSG_RESULT( [defined to be big endian] )
dnl             else
dnl                 AC_MSG_RESULT( [defined to be little endian] )
dnl             fi
dnl         else
dnl             AC_DEFINE( WORDS_BIGENDIAN, 1,
dnl                        [Define if byte order is bigendian] )
dnl             AC_MSG_RESULT( [Unknown CROSS_BIGENDIAN, $CROSS_BIGENDIAN! Assumed big endian] )
dnl         fi
dnl     else
dnl         AC_DEFINE( WORDS_BIGENDIAN, 1, [Define if byte order is bigendian] )
dnl         AC_MSG_RESULT( [Undefined CROSS_BIGENDIAN! Assumed big endian] )
dnl     fi
dnl else
    AC_C_BIGENDIAN
dnl fi

AC_CHECK_HEADERS(stdio.h stdlib.h string.h unistd.h sys/time.h sys/types.h sys/bitypes.h sys/stat.h fcntl.h)


dnl
dnl Check system headers for C99 intXX_t.
dnl
PAC_GET_STDINT_HEADER([mpe_c99_stdint_h])

if test "X$mpe_c99_stdint_h" != "X" ; then
    mpe_int8_t="int8_t"
    mpe_int16_t="int16_t"
    mpe_int32_t="int32_t"
    mpe_int64_t="int64_t"
else
dnl Since there isn't any intXX_t defined in the system headers.
dnl Define intXX_t based on basic integer types, char, short, int, long...
    PAC_GET_BASIC_INT_TYPES([mpe_int8_t], [mpe_int16_t],
                            [mpe_int32_t], [mpe_int64_t])
fi

dnl
dnl Check for PRIdXX
dnl
PAC_GET_STDINT_FORMATS([$mpe_int8_t], [$mpe_int16_t],
                        [$mpe_int32_t], [$mpe_int64_t],
                        [mpe_int8_fmt], [mpe_int16_fmt],
                        [mpe_int32_fmt], [mpe_int64_fmt],
                        [mpe_c99_pridxx_h])

dnl Create the output stdint.h for MPE logging
dnl PAC_OUTPUT_STDINT_HEADER([include/clog_stdint.h],
dnl                          [$mpe_c99_stdint_h], [$mpe_c99_pridxx_h],
dnl                          [$mpe_int8_t], [$mpe_int16_t],
dnl                          [$mpe_int32_t], [$mpe_int64_t],
dnl                          [$mpe_int8_fmt], [$mpe_int16_fmt],
dnl                          [$mpe_int32_fmt], [$mpe_int64_fmt])
mpe_c99_stdint_inc=""
mpe_c99_pridxx_inc=""
if test "X$mpe_c99_stdint_h" != "X" ; then
    mpe_c99_stdint_inc="#include <$mpe_c99_stdint_h>"
    if test "X$mpe_c99_pridxx_h" != "X" ; then
        if test "$mpe_c99_pridxx_h" != "$mpe_c99_stdint_h" ; then
            mpe_c99_pridxx_inc="#include <$mpe_c99_pridxx_h>"
        fi
    fi
fi
AC_SUBST(mpe_c99_stdint_inc)
AC_SUBST(mpe_c99_pridxx_inc)
AC_SUBST(mpe_int8_t)
AC_SUBST(mpe_int16_t)
AC_SUBST(mpe_int32_t)
AC_SUBST(mpe_int64_t)
AC_SUBST(mpe_int8_fmt)
AC_SUBST(mpe_int16_fmt)
AC_SUBST(mpe_int32_fmt)
AC_SUBST(mpe_int64_fmt)

if test "X$mpe_c99_stdint_h" != "X" ; then
    AC_MSG_CHECKING( [if <$mpe_c99_stdint_h> can be behind system headers] )
    AC_COMPILE_IFELSE( [ AC_LANG_PROGRAM( [
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#include <$mpe_c99_stdint_h>
                                          ],
                                          [ int64_t   ii64 = 0; ] )
                       ], [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
    if test "$mpe_link_ok" = "yes" ; then
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
        AC_DEFINE( FIX_STDINT_ORDER, 1,
                   [Define if stdint.h should be before system headers] )
    fi
fi

dnl Since sizeof(MPI_Aint) >= sizeof(void *), cannot use MPI_Aint.
dnl Define CLOG_Pint, where sizeof(CLOG_Pint) == sizeof(void *).
dnl Define CLOG_Pint to be used by MPI_Comm_xxx_attr in clog_commset.c.
AC_CHECK_SIZEOF(void *)
CLOG_PINT=mpe_int64_t
AC_MSG_CHECKING( [for address-sized integer] )
for bytelen in 8 16 32 64 ; do
    charlen=`expr $bytelen / 8`
    if test "$ac_cv_sizeof_void_p" = "$charlen" ; then
        CLOG_PINT=CLOG_int${bytelen}_t
        CLOG_SIZEOF_PINT=$charlen
        break;
    fi
done
AC_MSG_RESULT([$CLOG_PINT])
AC_SUBST(CLOG_PINT)

# If sizeof(mpi_aint) = sizeof(int), set this value
if test "$ac_cv_sizeof_int" = "$ac_cv_sizeof_void_p" ; then
    AC_DEFINE(SIZEOF_INT_IS_AINT,1,[define if sizeof(int) = sizeof(void*)])
fi



dnl ****   Checking for specific functions needed for the package.
 
if test -z "$nonopt" ; then nonopt=NONE ; fi
AC_CANONICAL_HOST
case "$host" in
   *cray*)
        if test "$MPE_BUILD_FORTRAN2C" = "yes" ; then
            AC_MSG_CHECKING( [for Cray's FCD string type] )
            AC_LINK_IFELSE( [ AC_LANG_PROGRAM( [#include <fortran.h>], [
                                char  *cstr;
                                int   cstrlen;
                                _fcd  fstr;
                                cstr      = _fcdtocp( fstr );
                                cstr_len  = _fcdlen( fstr );
                            ] ) ], [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE( HAVE_CRAY_FCD_STRING, 1,
                           [Define if CRAY's FCD string is found] )
            else
                AC_MSG_RESULT(no)
            fi
        fi
        ;;
esac

dnl snprintf may have been missing in headerfile,
dnl e.g. gcc -ansi remove gethostname's prototype
AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

dnl snprintf may have been missing in headerfile,
dnl e.g. gcc -ansi remove snprintf's prototype
AC_CHECK_FUNCS(snprintf)
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi

dnl mkstemp() is a better replacement for mktemp()
AC_HAVE_FUNCS(mkstemp)
if test "$ac_cv_func_mkstemp" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],mkstemp)
fi

dnl srand48 may have been missing in headerfile,
dnl e.g. gcc -ansi remove srand48's prototype
AC_CHECK_FUNCS(srand48)
if test "$ac_cv_func_srand48" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],srand48)
fi

dnl lrand48 may have been missing in headerfile,
dnl e.g. gcc -ansi remove lrand48's prototype
AC_CHECK_FUNCS(lrand48)
if test "$ac_cv_func_lrand48" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],lrand48)
fi


dnl check if uuid_generate exists
dnl AC_CHECK_HEADERS( uuid/uuid.h,
dnl     AC_MSG_CHECKING( [for uuid_generate] )
dnl     AC_TRY_COMPILE( [
dnl         #include <uuid/uuid.h>
dnl         ], [
dnl         uuid_t new_uuid;
dnl         uuid_generate( new_uuid );
dnl         ],
dnl         AC_DEFINE( HAVE_UUID_GENERATE, 1,
dnl                    [Define if uuid_generate() exists.] )
dnl         AC_MSG_RESULT(yes),
dnl         AC_MSG_RESULT(no)
dnl     )
dnl )

dnl check if Large File support is available for 32-bit OS
dnl AC_CHECK_HEADERS( unistd.h,
dnl     AC_MSG_CHECKING( [for large file support in 32-bit OS] )
dnl     AC_TRY_COMPILE( [
dnl         #define _LARGEFILE64_SOURCE 1
dnl         #include <unistd.h>
dnl         ], [
dnl         #ifndef _LFS64_LARGEFILE
dnl         #error no largefile support
dnl         #endif
dnl         ],
dnl         AC_DEFINE( _LARGEFILE64_SOURCE, 1,
dnl                    [Define if 64-bit file address support in 32-bit OS.] )
dnl         AC_DEFINE( _FILE_OFFSET_BITS, 64,
dnl                    [Define if 64-bit file address support in 32-bit OS.] )
dnl         AC_MSG_RESULT(yes),
dnl         AC_MSG_RESULT(none)
dnl     )
dnl )
AC_SYS_LARGEFILE
if test "$enable_largefile" != no ; then
    AC_MSG_CHECKING( [for O_LARGEFILE for open()] )
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([
#if defined( HAVE_SYS_TYPES_H )
#include <sys/types.h>
#endif
#if defined( HAVE_SYS_STAT_H )
#include <sys/stat.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
        ],[
        int fd;
        fd = open( "tmpfile", O_CREAT | O_LARGEFILE, 0644 );   
        ])
    ],[
        AC_DEFINE( HAVE_O_LARGEFILE, 1,
                   [Define to 1 if O_LARGEFILE flag for open() exists.] )
        AC_MSG_RESULT(yes)
    ],[
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM([
#define _LARGEFILE64_SOURCE
#if defined( HAVE_SYS_TYPES_H )
#include <sys/types.h>
#endif
#if defined( HAVE_SYS_STAT_H )
#include <sys/stat.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
            ],[
    int fd;
    fd = open( "tmpfile", O_CREAT | O_LARGEFILE, 0644 );   
            ])
        ],[
            AC_DEFINE( HAVE_O_LARGEFILE, 1,
                       [Define to 1 if O_LARGEFILE flag for open() exists.] )
            AC_DEFINE( _LARGEFILE64_SOURCE, 1,
                       [Define if 64-bit file address support in 32-bit OS.] )
            AC_MSG_RESULT( [needs _LARGEFILE64_SOURCE] )
        ],[
            AC_MSG_RESULT(no)
        ])
    ])
fi
        dnl end of AC_COMPILE_IFELSE with O_LARGEFILE
    dnl end of AC_COMPILE_IFELSE with O_LARGEFILE and _LARGEFILE64_SOURCE

AC_MSG_CHECKING( [for O_BINARY for open()] )
AC_LINK_IFELSE([
    AC_LANG_PROGRAM([
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif
    ],[
    int fd;
    fd = open( "tmpfile", O_CREAT | O_BINARY, 0644 );
    ])
],[
    AC_DEFINE( HAVE_O_BINARY, 1,
               [Define to 1 if O_BINARY flag for open() exists.] )
    AC_MSG_RESULT(yes)
],[
    AC_MSG_RESULT(no)
])

dnl   Determine if PMPI_Comm_create_keyval exists
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    AC_MSG_CHECKING( [for PMPI_Comm_create_keyval()] )
    PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                          [ int comm_keyval ], [
                          PMPI_Comm_create_keyval( MPI_COMM_NULL_COPY_FN,
                                                   MPI_COMM_NULL_DELETE_FN,
                                                   &comm_keyval, NULL )
                          ],
              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
    AC_MSG_RESULT($mpe_link_ok)
else
#   only MPICH2 has PMPI_Comm_{create|free}_keyval, PMPI_Comm_{set|get}_attr
    if test "$MPI_IMPL" = "MPICH2" ; then
        mpe_link_ok=yes
    else
        mpe_link_ok=no
    fi
fi
if test "$mpe_link_ok" = "yes" ; then
    AC_DEFINE( HAVE_PMPI_COMM_CREATE_KEYVAL, 1,
               [Define if PMPI_Comm_Create_keyval() available] )
fi

dnl   Determine if PMPI_Comm_free_keyval exists
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    AC_MSG_CHECKING( [for PMPI_Comm_free_keyval()] )
    PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                          [ int comm_keyval ], [
                          PMPI_Comm_free_keyval( &comm_keyval )
                          ],
              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
    AC_MSG_RESULT($mpe_link_ok)
else
#   only MPICH2 has PMPI_Comm_{create|free}_keyval, PMPI_Comm_{set|get}_attr
    if test "$MPI_IMPL" = "MPICH2" ; then
        mpe_link_ok=yes
    else
        mpe_link_ok=no
    fi
fi
if test "$mpe_link_ok" = "yes" ; then
    AC_DEFINE( HAVE_PMPI_COMM_FREE_KEYVAL, 1,
               [Define if PMPI_Comm_free_keyval() available] )
fi

dnl   Determine if PMPI_Comm_set_attr exists
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    AC_MSG_CHECKING( [for PMPI_Comm_set_attr()] )
    PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                          [ int comm_keyval ], [
                          PMPI_Comm_set_attr( MPI_COMM_WORLD, comm_keyval,
                                              (void *)5 )
                          ],
              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
    AC_MSG_RESULT($mpe_link_ok)
else
#   only MPICH2 has PMPI_Comm_{create|free}_keyval, PMPI_Comm_{set|get}_attr
    if test "$MPI_IMPL" = "MPICH2" ; then
        mpe_link_ok=yes
    else
        mpe_link_ok=no
    fi
fi
if test "$mpe_link_ok" = "yes" ; then
    AC_DEFINE( HAVE_PMPI_COMM_SET_ATTR, 1,
               [Define if PMPI_Comm_set_attr() available] )
fi

dnl   Determine if PMPI_Comm_get_attr exists
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    AC_MSG_CHECKING( [for PMPI_Comm_get_attr()] )
    PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                          [ int comm_keyval, comm_val, istatus ], [
                          PMPI_Comm_get_attr( MPI_COMM_WORLD, comm_keyval,
                                              &comm_val, &istatus )
                          ],
              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
    AC_MSG_RESULT($mpe_link_ok)
else
#   only MPICH2 has PMPI_Comm_{create|free}_keyval, PMPI_Comm_{set|get}_attr
    if test "$MPI_IMPL" = "MPICH2" ; then
        mpe_link_ok=yes
    else
        mpe_link_ok=no
    fi
fi
if test "$mpe_link_ok" = "yes" ; then
    AC_DEFINE( HAVE_PMPI_COMM_GET_ATTR, 1,
               [Define if PMPI_Comm_get_attr() available] )
fi

dnl Check if pthread exists
if test "$enable_threadlogging" = "yes" ; then
    AC_CHECK_HEADERS( pthread.h, [
         AC_CHECK_LIB( pthread, [pthread_mutex_lock],
                       [ have_pthread_in_cc=yes ],
                       [ have_pthread_in_cc=no ] )
    ] )
    if test "$have_pthread_in_cc" = "yes" ; then
        AC_DEFINE( HAVE_LIBPTHREAD, 1,
                   [Define if libpthread.a(so) is defined] )
    fi
fi

dnl Assume there is no thread support, by default.
PTHREAD_IN_MPI_DEF="#undef HAVE_PTHREAD_IN_MPI"
THREAD_LOG_LIBS=""
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$have_pthread_in_cc" = "yes" \
         -a "$enable_threadlogging" = "yes" ; then
        AC_MSG_CHECKING( [for pthread_mutex_lock/unlock in MPI] )
dnl     Check MPI_CC/MPI_LIBS without -lpthread can resolve pthread symbols,
dnl     no need to add -lpthread during linking.
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS],
                              [$MPI_LIBS],
                              [#include <pthread.h>],
                              [ pthread_mutex_t pthd_mutex ],
                              [
                                pthread_mutex_lock(&pthd_mutex);
                                pthread_mutex_unlock(&pthd_mutex);
                              ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            PTHREAD_IN_MPI_DEF="#define HAVE_PTHREAD_IN_MPI 1"
            THREAD_LOG_LIBS=""
        else
dnl         Check MPI_CC/MPI_LIBS with -lpthread can resolve pthread symbols,
dnl         If so, add -lpthread during linking.
            PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS],
                                  [$MPI_LIBS -lpthread],
                                  [#include <pthread.h>],
                                  [ pthread_mutex_t pthd_mutex ],
                                  [
                                    pthread_mutex_lock(&pthd_mutex);
                                    pthread_mutex_unlock(&pthd_mutex);
                                  ],
                                  [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
            if test "$mpe_link_ok" = "yes" ; then
                AC_MSG_RESULT(yes)
                PTHREAD_IN_MPI_DEF="#define HAVE_PTHREAD_IN_MPI 1"
                THREAD_LOG_LIBS="-lpthread"
            else
                AC_MSG_RESULT(no)
            fi
        fi
    fi
else
dnl MPICH1 has no MPI_Init_thread(), set HAVE_PTHREAD_IN_MPI to Null.
    if test "$MPI_IMPL" = "MPICH2" ; then
        if test "$enable_threadlogging" = "yes" ; then
            if test "$MPICH_THREAD_LEVEL" = "MPI_THREAD_MULTIPLE" \
                -a  "$MPICH2_THREAD_PACKAGE" = "pthreads" ; then
                PTHREAD_IN_MPI_DEF="#define HAVE_PTHREAD_IN_MPI 1"
                THREAD_LOGGING_LIBS="-lpthread"
            fi
        fi
    fi
fi
AC_SUBST(PTHREAD_IN_MPI_DEF)
AC_SUBST(THREAD_LOG_LIBS)


AC_SUBST(MPE_BUILD_FORTRAN2C)
AC_SUBST(CC)
AC_SUBST(CLINKER)
AC_SUBST(CFLAGS)
AC_SUBST(MPI_CC)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LIBS)
AC_SUBST(PROF_LIBNAME)

AC_OUTPUT( Makefile src/Makefile \
           include/clog_inttypes.h include/mpi_null.h include/mpe_log_thread.h \
           etc/mpe_log.conf etc/mpe_nolog.conf etc/mpe_log_postlib.conf )
