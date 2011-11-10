[#] start of __file__
dnl MPICH2_SUBCFG_BEFORE=src_mpid_common_sched
dnl MPICH2_SUBCFG_BEFORE=src_mpid_common_datatype
dnl MPICH2_SUBCFG_BEFORE=src_mpid_common_thread

dnl _PREREQ handles the former role of mpich2prereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_src_mpid_ch3],[
AM_CONDITIONAL([BUILD_CH3],[test "$device_name" = "ch3"])

dnl this subconfigure.m4 handles the configure work for the ftb subdir too
dnl this AM_CONDITIONAL only works because enable_ftb is set very early on by
dnl autoconf's argument parsing code.  The "action-if-given" from the
dnl AC_ARG_ENABLE has not yet run
AM_CONDITIONAL([BUILD_CH3_UTIL_FTB],[test "x$enable_ftb" = "xyes"])

AM_COND_IF([BUILD_CH3],[

# Set a value for the maximum processor name.
MPID_MAX_PROCESSOR_NAME=128

# code that formerly lived in setup_device.args
if test -z "${device_args}" ; then
    device_args="nemesis"
fi
channel_name=`echo ${device_args} | sed -e 's/:.*$//'`
# observe the [] quoting below
channel_args=`echo ${device_args} | sed -e 's/^[[^:]]*//' -e 's/^://'`

#
# reset DEVICE so that it (a) always includes the channel name, and (b) does not include channel options
#
DEVICE="${device_name}:${channel_name}"

dir=${master_top_srcdir}/src/mpid/${device_name}/channels/${channel_name}
if test ! -d $dir ; then
    echo "ERROR: ${dir} does not exist"
    exit 1
fi
## TODO this code can/should be eliminated
file=${dir}/setup_channel
if test -f $file ; then
    echo sourcing $file
    . $file
fi

export channel_name
export channel_args
AC_SUBST(device_name)
AC_SUBST(channel_name)

# end old setup_device code

if test ! -d $srcdir/src/mpid/ch3/channels/${channel_name} ; then
    AC_MSG_ERROR([Channel ${channel_name} is unknown])
elif test ! -f $srcdir/src/mpid/ch3/channels/${channel_name}/subconfigure.m4 ; then
    AC_MSG_ERROR([Channel ${channel_name} has no subconfigure.m4])
fi

# the CH3 device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes

])dnl end AM_COND_IF(BUILD_CH3,...)
])dnl end PREREQ
AC_DEFUN([PAC_SUBCFG_BODY_src_mpid_ch3],[
AM_COND_IF([BUILD_CH3],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR CH3 DEVICE])

# ----------------------------------------------------------------------------
# include ftb functionality
# ----------------------------------------------------------------------------
AC_ARG_ENABLE([ftb],
  [AS_HELP_STRING([[--enable-ftb]],
    [Enable FTB support (default is no)])],
  [AC_DEFINE([ENABLE_FTB], 1, [Define if FTB is enabled])
   PAC_SET_HEADER_LIB_PATH([ftb])
   PAC_CHECK_HEADER_LIB_FATAL([ftb], [libftb.h], [ftb], [FTB_Connect])]
)

AC_ARG_WITH(ch3-rank-bits, [--with-ch3-rank-bits=16/32     Number of bits allocated to the rank field (16 or 32)],
			   [ rankbits=$withval ],
			   [ rankbits=16 ])
if test "$rankbits" != "16" -a "$rankbits" != "32" ; then
   AC_MSG_ERROR(Only 16 or 32-bit ranks are supported)
fi
AC_DEFINE_UNQUOTED(CH3_RANK_BITS,$rankbits,[Define the number of CH3_RANK_BITS])

AC_CHECK_HEADERS(assert.h limits.h string.h sys/types.h sys/uio.h uuid/uuid.h \
    time.h ctype.h unistd.h arpa/inet.h sys/socket.h)

# net/if.h requires special handling on darwin.  The following code is
# straight out of the autoconf-2.63 manual.  Also, sys/socket.h (above)
# is a prerequisite.
AC_CHECK_HEADERS([net/if.h], [], [],
[#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

# Check for special types
AC_TYPE_PID_T

# Check for functions
AC_CHECK_FUNCS(inet_pton)
AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

AC_CHECK_FUNCS(CFUUIDCreate uuid_generate time)

AC_CACHE_CHECK([whether CPP accepts variable length argument lists],
pac_cv_have_cpp_varargs,[
AC_TRY_COMPILE([
#include <stdio.h>
#define MY_PRINTF(rank, fmt, args...)  printf("%d: " fmt, rank, ## args)
],[
MY_PRINTF(0, "hello");
MY_PRINTF(1, "world %d", 3);
], pac_cv_have_cpp_varargs=yes, pac_cv_have_cpp_varargs=no)
])
if test $pac_cv_have_cpp_varargs = "yes" ; then
    AC_DEFINE(HAVE_CPP_VARARGS,,[Define if CPP supports macros with a variable number arguments])
fi

AC_C_BIGENDIAN

# If we need the socket code, see if we can use struct ifconf
# Some systems require special compile options or definitions, or 
# special header files.
#
# To simplify this sequence of tests, we set
#   pac_found_struct_ifconf 
# to yes when we figure out how to get struct ifconf
pac_found_struct_ifconf=no
#
# sys/socket.h is needed on Solaris
AC_CACHE_CHECK([whether we can use struct ifconf],
pac_cv_have_struct_ifconf,[
AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest;],
[pac_cv_have_struct_ifconf=yes;pac_found_struct_ifconf=yes],
 pac_cv_have_struct_ifconf=no)])

if test "$pac_found_struct_ifconf" = "no" ; then
    # Try again with _SVID_SOURCE
    AC_CACHE_CHECK([whether we can use struct ifconf with _SVID_SOURCE],
pac_cv_have_struct_ifconf_with_svid,[
AC_TRY_COMPILE([
#define _SVID_SOURCE
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest;],
pac_cv_have_struct_ifconf_with_svid=yes;pac_found_struct_ifconf=yes,
pac_cv_have_struct_ifconf_with_svid=no)])
    if test "$pac_cv_have_struct_ifconf_with_svid" = yes ; then
        AC_DEFINE(USE_SVIDSOURCE_FOR_IFCONF,1,[Define if _SVID_SOURCE needs to be defined for struct ifconf])
    fi
fi

if test "$pac_found_struct_ifconf" = "no" ; then
    # Try again with undef _POSIX_C_SOURCE
    AC_CACHE_CHECK([whether we can use struct ifconf without _POSIX_C_SOURCE],
pac_cv_have_struct_ifconf_without_posix,[
AC_TRY_COMPILE([
#undef _POSIX_C_SOURCE
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest;],
pac_cv_have_struct_ifconf_without_posix=yes;pac_found_struct_ifconf=yes,
pac_cv_have_struct_ifconf_without_posix=no)])
    if test "$pac_cv_have_struct_ifconf_without_posix" = yes ; then
        AC_DEFINE(USE_NOPOSIX_FOR_IFCONF,1,[Define if _POSIX_C_SOURCE needs to be undefined for struct ifconf])
    fi
fi

if test "$pac_found_struct_ifconf" = "no" ; then
    # Try again with _ALL_SOURCE
    AC_CACHE_CHECK([whether we can use struct ifconf with _ALL_SOURCE],
pac_cv_have_struct_ifconf_with_svid,[
AC_TRY_COMPILE([
#define _ALL_SOURCE
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest;],
pac_cv_have_struct_ifconf_with_all_source=yes;pac_found_struct_ifconf=yes,
pac_cv_have_struct_ifconf_with_all_source=no)])
    if test "$pac_cv_have_struct_ifconf_with_all_source" = yes ; then
        AC_DEFINE(USE_ALL_SOURCE_FOR_IFCONF,1,[Define if _ALL_SOURCE needs to be defined for struct ifconf])
    fi
fi

if test "$pac_found_struct_ifconf" = "yes" ; then
    AC_DEFINE(HAVE_STRUCT_IFCONF,1,[Define if struct ifconf can be used])
fi

])dnl end AM_COND_IF(BUILD_CH3,...)
])dnl end _BODY

[#] end of __file__
