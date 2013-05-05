[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/pmi

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_SIMPLE],[test "x$pmi_name" = "xsimple"])

AM_COND_IF([BUILD_PMI_SIMPLE],[
if test "$enable_pmiport" != "no" ; then
   enable_pmiport=yes
fi
AC_CHECK_HEADERS(unistd.h string.h stdlib.h sys/socket.h strings.h assert.h arpa/inet.h)
dnl Use snprintf if possible when creating messages
AC_CHECK_FUNCS(snprintf)
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi
AC_CHECK_FUNCS(strncasecmp)

#
# PM's that need support for a port can set the environment variable
# NEED_PMIPORT in their setup_pm script.
if test "$NEED_PMIPORT" = "yes" -a "$enable_pmiport" != "yes" ; then
    AC_MSG_WARN([The process manager requires the pmiport capability.  Do not specify --disable-pmiport.])
    enable_pmiport=yes
fi
#
if test "$enable_pmiport" = "yes" ; then
    # Check for the necessary includes and functions
    missing_headers=no
    AC_CHECK_HEADERS([	\
	sys/types.h	\
	sys/param.h	\
	sys/socket.h	\
	netinet/in.h	\
	netinet/tcp.h	\
	sys/un.h	\
	netdb.h		\
	],,missing_headers=yes )
    AC_SEARCH_LIBS(socket,socket)
    AC_SEARCH_LIBS(gethostbyname,nsl)
    missing_functions=no
    AC_CHECK_FUNCS(socket setsockopt gethostbyname,,missing_functions=yes)
    
    if test "$missing_functions" = "no" ; then
        AC_DEFINE(USE_PMI_PORT,1,[Define if access to PMI information through a port rather than just an fd is allowed])
    else
        AC_MSG_ERROR([Cannot build simple PMI with support for an IP port because of missing functions])
    fi
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
]
typedef struct { double a; int b; } socklen_t;,[socklen_t a;a.a=1.0;],pac_cv_have_socklen_t=no,pac_cv_have_socklen_t=yes)])
if test "$pac_cv_have_socklen_t" = no ; then
    AC_DEFINE(socklen_t,int,[Define if socklen_t is not defined])
fi
# Check for h_addr or h_addr_list
AC_CACHE_CHECK([whether struct hostent contains h_addr_list],
pac_cv_have_haddr_list,[
AC_TRY_COMPILE([
#include <netdb.h>],[struct hostent hp;hp.h_addr_list[0]=0;],
pac_cv_have_haddr_list=yes,pac_cv_have_haddr_list=no)])
if test "$pac_cv_have_haddr_list" = "yes" ; then
    AC_DEFINE(HAVE_H_ADDR_LIST,1,[Define if struct hostent contains h_addr_list])
fi
PAC_C_GNU_ATTRIBUTE
])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
