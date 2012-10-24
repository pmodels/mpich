[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/common/sock

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_MPID_COMMON_SOCK_POLL],[test "X$build_mpid_common_sock_poll" = "Xyes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_MPID_COMMON_SOCK_POLL],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/mpid/common/sock/poll])

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

])dnl end AM_COND_IF(BUILD_MPID_COMMON_SOCK_POLL,...)
])dnl end _BODY
dnl
[#] end of __file__
