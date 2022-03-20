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
    
    ])dnl end AM_COND_IF(BUILD_CH3U_SOCK,...)
])dnl end _BODY

[#] end of __file__
