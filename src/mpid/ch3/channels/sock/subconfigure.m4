[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sock
dnl MPICH_SUBCFG_BEFORE=src/mpid/ch3/util/sock

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_CH3_SOCK],[test "X$device_name" = "Xch3" -a "X$channel_name" = "Xsock"])
    AM_COND_IF([BUILD_CH3_SOCK],[
        AC_MSG_NOTICE([RUNNING PREREQ FOR ch3:sock])
        # this channel depends on the sock utilities
        build_ch3u_sock=yes

        MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
        MPID_CH3I_CH_HCOLL_BCOL="basesmuma,basesmuma,ptpcoll"

        # code that formerly lived in setup_args
        #
        # Variables of interest...
        #
        # $with_device - device name and arguments
        # $device_name - name of the device
        # $device_args - contains name of channel select plus an channel args
        # $channel_name - name of the channel
        # $master_top_srcdir - top-level source directory
        # $master_top_builddir - top-level build directory
        # $ac_configure_args - all arguments passed to configure
        #
        
        found_dir="no"
        for sys in $devsubsystems ; do
            if test "$sys" = "src/mpid/common/sock" ; then
               found_dir="yes"
               break
            fi
        done
        if test "$found_dir" = "no" ; then
           devsubsystems="$devsubsystems src/mpid/common/sock"
        fi
        
        # FIXME: The setup file has a weird requirement that it needs to be
        # run *before* the MPICH device (not the setup directory itself) is
        # configured, but the actual configuration of the associated directory
        # needs to be done *after* the device is configured.
        file=${master_top_srcdir}/src/mpid/common/sock/setup
        if test -f $file ; then
           echo sourcing $file
           . $file
        fi
        
        pathlist=""
        pathlist="$pathlist src/mpid/${device_name}/channels/${channel_name}/include"
        pathlist="$pathlist src/util/wrappers"
        ## TODO delete this -I junk
        ##for path in $pathlist ; do
        ##    CPPFLAGS="$CPPFLAGS -I${master_top_builddir}/${path} -I${master_top_srcdir}/${path}"
        ##done

# Adding this prevents the pesky "(strerror() not found)" problem, which can be
# very frustrating in the sock code, the most likely to receive an error code
# from the system. [goodell@ 2008-01-10]
AC_CHECK_FUNCS([strerror])

AC_ARG_ENABLE([sock-debug],
              [--enable-sock-debug - Turn on tests of the socket data structures],
              [],
              [enable_sock_debug=no])

if test "$enable_sock_debug" = yes ; then
    AC_DEFINE(USE_SOCK_VERIFY,1,[Define it the socket verify macros should be enabled])
fi

AC_CHECK_FUNC([poll],[pac_cv_have_func_poll=yes],[pac_cv_have_func_poll=no])
if test "X$pac_cv_have_func_poll" = "Xno" ; then
    if test -f /sw/include/sys/poll.h ; then
        dnl This is for Mac OSX (Darwin) which doesn't have a poll function is
        dnl the standard libraries.  Instead we use a contributed library found
        dnl off in la la land (/sw).
        dnl
        dnl I don't think the above comment is true for modern Darwin (goodell@ 2010-06-01)
        CFLAGS="$CFLAGS -I/sw/include"
        LIBS="$LIBS -L/sw/lib -lpoll"
    else
        AC_MSG_ERROR([This device requires the poll function])
    fi
fi

AC_CHECK_HEADERS([assert.h errno.h fcntl.h limits.h netdb.h netinet/in.h netinet/tcp.h])
AC_CHECK_HEADERS([poll.h stdlib.h sys/param.h sys/poll.h sys/types.h sys/uio.h unistd.h])

AC_MSG_CHECKING([if struct poll is defined]);
AC_TRY_COMPILE([
#if defined(HAVE_POLL_H)
#include <poll.h>
#endif
#if defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
],[
struct pollfd pollfd;
pollfd.fd = -1;
],[
AC_MSG_RESULT([yes])
],[
AC_MSG_RESULT([no])
AC_MSG_ERROR([This device requires the poll function])
])

AC_MSG_CHECKING([if a simple program using poll() can be compiled]);
AC_TRY_COMPILE([
#if defined(HAVE_POLL_H)
#include <poll.h>
#endif
#if defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
],[
struct pollfd pollfds[2];
int n_fds;
pollfds[0].fd = -1;
pollfds[1].fd = -1;
n_fds = poll(pollfds, 2, -1);
],[
AC_MSG_RESULT([yes])
],[
AC_MSG_RESULT([no])
AC_MSG_ERROR([This device requires the poll function])
])

dnl
dnl needed on AIX
dnl
AC_MSG_CHECKING([whether bit fields work in ip.h]);
AC_TRY_COMPILE([
#include <netinet/tcp.h>
],[
int i;
],[
AC_MSG_RESULT([yes])
bit_fields=yes
],[
AC_MSG_RESULT([no])
bit_fields=no
])
if test "$bit_fields" = "no" ; then
     AC_MSG_RESULT([Adding -D_NO_BITFIELDS to CFLAGS])
     CFLAGS="$CFLAGS -D_NO_BITFIELDS"
fi


AC_CHECK_FUNCS([gethostname])
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

AC_SEARCH_LIBS([socket],[socket])
AC_SEARCH_LIBS([gethostbyname],[nsl])

# Check first for sys/socket.h .  We check not only for existence but whether
# it can be compiled (!), as we have seen some problems with this.
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[int a;],ac_cv_header_sys_socket_h=yes,ac_cv_header_sys_socket_h=no)
if test "$ac_cv_header_sys_socket_h" = yes ; then
    AC_DEFINE(HAVE_SYS_SOCKET_H,1,[Define if you have the <sys/socket.h> header file.])
fi
# Check for socklen_t .  If undefined, define it as int
# (note the conditional inclusion of sys/socket.h)
AC_CACHE_CHECK([whether socklen_t is defined (in sys/socket.h if present)],
pac_cv_have_socklen_t,[
AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
typedef struct { double a; int b; } socklen_t;],
[socklen_t a;a.a=1.0;],
[pac_cv_have_socklen_t=no],
[pac_cv_have_socklen_t=yes])])
if test "X$pac_cv_have_socklen_t" = Xno ; then
    AC_DEFINE([socklen_t],[int],[Define if socklen_t is not defined])
fi

    ])
])dnl
dnl
dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH3_SOCK],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:sock])
# code that formerly lived in configure.ac

dnl AC_CHECK_HEADER(net/if.h) fails on Solaris; extra header files needed
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
],,lac_cv_header_net_if_h=yes,lac_cv_header_net_if_h=no)

# FIXME why doesn't this use the proper machinery?
echo "checking for net/if.h... $lac_cv_header_net_if_h"

if test "$lac_cv_header_net_if_h" = "yes" ; then
    AC_DEFINE(HAVE_NET_IF_H, 1, [Define if you have the <net/if.h> header file.])
fi

AC_CHECK_HEADERS(				\
	netdb.h					\
	sys/ioctl.h				\
	sys/socket.h				\
	sys/sockio.h				\
	sys/types.h				\
	errno.h)

# netinet/in.h often requires sys/types.h first.  With AC 2.57, check_headers
# does the right thing, which is to test whether the header is found 
# by the compiler, but this can cause problems when the header needs 
# other headers.  2.57 changes the syntax (!) of check_headers to allow 
# additional headers.
AC_CACHE_CHECK([for netinet/in.h],ac_cv_header_netinet_in_h,[
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netinet/in.h>],[int a=0;],
    ac_cv_header_netinet_in_h=yes,
    ac_cv_header_netinet_in_h=no)])
if test "$ac_cv_header_netinet_in_h" = yes ; then
    AC_DEFINE(HAVE_NETINET_IN_H,1,[Define if netinet/in.h exists])
fi

])dnl end AM_COND_IF(BUILD_CH3_SOCK,...)
])dnl end _BODY

[#] end of __file__
