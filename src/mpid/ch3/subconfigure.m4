[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
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
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
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
   PAC_PUSH_FLAG(LIBS)
   PAC_CHECK_HEADER_LIB_FATAL([ftb], [libftb.h], [ftb], [FTB_Connect])
   PAC_APPEND_FLAG([-lftb],[WRAPPER_LIBS])
   PAC_POP_FLAG(LIBS)]
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

# ensure that atomic primitives are available
AC_MSG_CHECKING([for OpenPA atomic primitive availability])

# Double check that we actually have a present and working OpenPA
# configuration.  This must be AC_COMPILE_IFELSE instead of the stronger
# AC_LINK_IFELSE because the OpenPA library will typically not be
# completely built by this point.
#
# This test was taken from sanity.c in the OpenPA test suite.
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <opa_primitives.h> /* will include pthread.h if present and needed */
]],[[
    OPA_int_t a, b;
    int c;
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
    pthread_mutex_t shm_lock;
    OPA_Interprocess_lock_init(&shm_lock, 1/*isLeader*/);
#endif

    OPA_store_int(&a, 0);
    OPA_store_int(&b, 1);
    OPA_add_int(&a, 10);
    OPA_assert(10 == OPA_load_int(&a));
    c = OPA_cas_int(&a, 10, 11);
    OPA_assert(10 == c);
    c = OPA_swap_int(&a, OPA_load_int(&b));
    OPA_assert(11 == c);
    OPA_assert(1 == OPA_load_int(&a));
]])],
openpa_present_and_working=yes,
openpa_present_and_working=no)

if test "$openpa_present_and_working" = yes ; then
    AC_PREPROC_IFELSE([
    AC_LANG_SOURCE([
#include <opa_primitives.h>
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
#error "lock-based emulation is currently in use"
#endif
    ])
],using_emulated_atomics=no,using_emulated_atomics=yes)

    if test "$using_emulated_atomics" = "yes" ; then
        AC_PREPROC_IFELSE([
        AC_LANG_SOURCE([
#include <opa_primitives.h>
/* may also be undefined in older (pre-r106) versions of OPA */
#if !defined(OPA_EXPLICIT_EMULATION)
#error "lock-based emulation was automatic, not explicit"
#endif
])
],[atomics_explicitly_emulated=yes],[atomics_explicitly_emulated=no])
        if test "$atomics_explicitly_emulated" = "yes" ; then
            AC_MSG_RESULT([yes (emulated)])
        else
            AC_MSG_RESULT([no])
            AC_MSG_ERROR([
The ch3 device was selected yet no native atomic primitives are
available on this platform.  OpenPA can emulate atomic primitives using
locks by specifying --with-atomic-primitives=no but performance will be
very poor.  This override should only be specified for correctness
testing purposes.])
        fi
    else
        AC_MSG_RESULT([yes])
    fi
else
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([
The ch3 devies was selected yet a set of working OpenPA headers
were not found.  Please check the OpenPA configure step for errors.])
fi

AC_C_BIGENDIAN

])dnl end AM_COND_IF(BUILD_CH3,...)
])dnl end _BODY

[#] end of __file__
