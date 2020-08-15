[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH3],[test "$device_name" = "ch3"])

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

dir=${main_top_srcdir}/src/mpid/${device_name}/channels/${channel_name}
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
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH3],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR CH3 DEVICE])

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

AC_C_BIGENDIAN

])dnl end AM_COND_IF(BUILD_CH3,...)
])dnl end _BODY

[#] end of __file__
