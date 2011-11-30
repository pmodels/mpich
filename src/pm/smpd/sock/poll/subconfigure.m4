[#] start of __file__
dnl MPICH2_SUBCFG_AFTER=src/pm/smpd

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AC_ARG_ENABLE([sock-debug],
              [--enable-sock-debug - Turn on tests of the socket data structures],
              [],
              [enable_sock_debug=no])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([SMPD_SOCK_IS_POLL],[test "X$with_smpd_sock" = "Xpoll"])

AC_ARG_VAR([TCP_LIBS],[add libraries to SMPD's sockets poll link line (deprecated, please don't use)])

AM_COND_IF([BUILD_PM_SMPD],[
AM_COND_IF([SMPD_SOCK_IS_POLL],[
dnl _XOPEN_SOURCE_EXTENDED=1 is required for Solaris to build properly */
dnl (TEMPORARY - Setting the Unix source type must happen before 
dnl any tests are performed to ensure that all configure tests use the
dnl same environment.  In particular, under AIX, this causes 
dnl struct timespec to *not* be defined, 
dnl CFLAGS="$CFLAGS -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED=1"

AC_CHECK_FUNC(poll,pac_cv_have_func_poll=yes,pac_cv_have_func_poll=no)
if test "$pac_cv_have_func_poll" != yes ; then
    if test -f /sw/include/sys/poll.h ; then
        dnl This is for Mac OSX (Darwin) which doesn't have a poll function is the standard libraries.  Instead we use a
        dnl  contributedlibrary found off in la la land (/sw).
        CFLAGS="$CFLAGS -I/sw/include"
        TCP_LIBS="$TCP_LIBS -L/sw/lib -lpoll"
    else
        AC_MSG_ERROR([This device requires the poll function])
    fi
fi

AC_CHECK_HEADERS([assert.h errno.h fcntl.h limits.h netdb.h netinet/in.h])
AC_CHECK_HEADERS([netinet/tcp.h poll.h stdlib.h sys/param.h sys/poll.h])
AC_CHECK_HEADERS([sys/types.h sys/uio.h unistd.h])

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
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],[gethostname])
fi

saveLIBS="$LIBS"
LIBS=""
AC_SEARCH_LIBS([socket],[socket],[],[],[$saveLIBS])
AC_SEARCH_LIBS([gethostbyname],[nsl],[],[],[$saveLIBS])
TCP_LIBS="$TCP_LIBS $LIBS"
LIBS="$saveLIBS $LIBS"

# Check first for sys/socket.h .  We check not only for existence but whether
# it can be compiled (!), as we have seen some problems with this.
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[int a;],[ac_cv_header_sys_socket_h=yes],[ac_cv_header_sys_socket_h=no])
if test "$ac_cv_header_sys_socket_h" = yes ; then
    AC_DEFINE([HAVE_SYS_SOCKET_H],[1],[Define if you have the <sys/socket.h> header file.])
fi
# Check for socklen_t .  If undefined, define it as int
# (note the conditional inclusion of sys/socket.h)
AC_CACHE_CHECK([whether socklen_t is defined (in sys/socket.h if present)],
[pac_cv_have_socklen_t],[
AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
typedef struct { double a; int b; } socklen_t;],
[socklen_t a;a.a=1.0;],
[pac_cv_have_socklen_t=no],
[pac_cv_have_socklen_t=yes])])
if test "$pac_cv_have_socklen_t" = no ; then
    AC_DEFINE([socklen_t],[int],[Define if socklen_t is not defined])
fi

])dnl end AM_COND_IF(SMPD_SOCK_IS_POLL,...)

])dnl end AM_COND_IF(BUILD_PM_SMPD,...)
])dnl end _BODY
dnl
[#] end of __file__
