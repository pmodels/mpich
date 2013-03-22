[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    # channels (like ch3:sock) that want to use this package should set build_ch3u_sock=yes
    AM_CONDITIONAL([BUILD_CH3_UTIL_SOCK],[test "x$build_ch3u_sock" = "xyes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_UTIL_SOCK],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR CH3U SOCK CODE])

    AC_CHECK_HEADERS(assert.h limits.h string.h sys/types.h sys/uio.h \
        time.h ctype.h unistd.h arpa/inet.h sys/socket.h net/if.h)

    # Check for special types
    AC_TYPE_PID_T

    # Check for functions
    AC_CHECK_FUNCS(inet_pton)
    AC_CHECK_FUNCS(gethostname)
    if test "$ac_cv_func_gethostname" = "yes" ; then
        # Do we need to declare gethostname?
        PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
    fi

    dnl AC_CHECK_FUNCS(CFUUIDCreate uuid_generate time)
    dnl AC_SEARCH_LIBS(uuid_generate, uuid)
    
    # If we need the socket code, see if we can use struct ifconf
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
pac_cv_have_struct_ifconf=yes,pac_cv_have_struct_ifconf=no)])

    if test "$pac_cv_have_struct_ifconf" = "no" ; then
        # Try again with _SVID_SOURCE
        AC_CACHE_CHECK([whether we can use struct ifconf with _SVID_SOURCE],
                       [pac_cv_have_struct_ifconf_with_svid],[
AC_TRY_COMPILE([
#define _SVID_SOURCE
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest;],
    pac_cv_have_struct_ifconf_with_svid=yes,
    pac_cv_have_struct_ifconf_with_svid=no)])
        if test "$pac_cv_have_struct_ifconf_with_svid" = yes ; then
            AC_DEFINE(USE_SVIDSOURCE_FOR_IFCONF,1,[Define if _SVID_SOURCE needs to be defined for struct ifconf])
        fi
    fi

    if test "$pac_cv_have_struct_ifconf" = "no" -a \
            "$pac_cv_have_struct_ifconf_with_svid" = "no" ; then
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
pac_cv_have_struct_ifconf_without_posix=yes,
pac_cv_have_struct_ifconf_without_posix=no)])
        if test "$pac_cv_have_struct_ifconf_without_posix" = yes ; then
            AC_DEFINE(USE_NOPOSIX_FOR_IFCONF,1,[Define if _POSIX_C_SOURCE needs to be undefined for struct ifconf])
        fi
    fi

    if test "$pac_cv_have_struct_ifconf" = "yes" -o \
            "$pac_cv_have_struct_ifconf_with_svid" = "yes" -o \
            "$pac_cv_have_struct_ifconf_without_posix" ; then
        AC_DEFINE(HAVE_STRUCT_IFCONF,1,[Define if struct ifconf can be used])
    fi

    ])dnl end AM_COND_IF(BUILD_CH3U_SOCK,...)
])dnl end _BODY

[#] end of __file__
