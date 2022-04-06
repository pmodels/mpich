##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AC_PREREQ([2.69])

AC_INIT([PMI], [1.2])

AC_CONFIG_AUX_DIR(confdb)
AC_CONFIG_MACRO_DIR(confdb)
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.12.3 subdir-objects no-dist])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_PROG_AR

AC_PROG_CC
AM_PROG_CC_C_O

PAC_C_NO_COMMON

LT_PREREQ([2.2.6])

PAC_PUSH_FLAG([CFLAGS])
LT_INIT()
PAC_POP_FLAG([CFLAGS])

AC_CONFIG_HEADERS([include/pmi_config.h])

AC_ARG_ENABLE([embedded],
    AS_HELP_STRING([--enable-embedded], [Build in embedded mode]),
    [enable_embedded=yes],
    [enable_embedded=no])
AM_CONDITIONAL([EMBEDDED_MODE], [test "$enable_embedded" = "yes"])

if test "$enable_embedded" = "yes" ; then
    AC_DEFINE([EMBEDDED_MODE], 1, [define if build in embedded mode])
fi

PAC_CHECK_VISIBILITY
if test -n "$VISIBILITY_CFLAGS" ; then
    CFLAGS="$CFLAGS $VISIBILITY_CFLAGS"
fi

AH_BOTTOM([
#if defined(HAVE_VISIBILITY) && !defined(EMBEDDED_MODE)
#define PMI_API_PUBLIC __attribute__((visibility ("default")))
#else
#define PMI_API_PUBLIC
#endif
])

AC_ARG_WITH([thread-package],
    AS_HELP_STRING([--with-thread-package], [whether to enable threads]))

case "$with_thread_package" in
    no|none)
        :
        ;;
    *)
        AC_DEFINE([HAVE_THREADS], 1, [Define if MPL supports threads])
        ;;
esac


# MPL
m4_define([mpl_embedded_dir],[mpl])

mpl_srcdir=
AC_SUBST([mpl_srcdir])
mpl_includedir=
AC_SUBST([mpl_includedir])
mpl_lib=
AC_SUBST([mpl_lib])

if test "$enable_embedded" = "yes" ; then
    pac_skip_mpl_lib=yes
fi
PAC_CONFIG_MPL

# check "mpi.h" for MPI_MAX_PORT_NAME
if test "$FROM_MPICH" = "yes" ; then
    PAC_APPEND_FLAG([-I${main_top_srcdir}/src/include], [CPPFLAGS])
    PAC_APPEND_FLAG([-I${main_top_builddir}/src/include], [CPPFLAGS])
    AC_DEFINE([HAVE_MPI_H], 1, [define if we have mpi.h])
else
    AC_CHECK_HEADER([mpi.h])
fi

if test "$enable_error_checking" != "no" ; then
    AC_DEFINE([HAVE_ERROR_CHECKING], 1, [Define to enable error checking])
fi

AC_ARG_ENABLE(pmiport,
[--enable-pmiport - Allow PMI interface to use a host-port pair to contact
                   for PMI services],,enable_pmiport=default)

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
    missing_functions=no
    AC_SEARCH_LIBS(socket,socket,,[missing_functions=yes])
    AC_SEARCH_LIBS(gethostbyname,nsl,,[missing_functions=yes])
    AC_SEARCH_LIBS(setsockopt,,,[missing_functions=yes])

    if test "$missing_functions" = "no" ; then
        AC_DEFINE(USE_PMI_PORT,1,[Define if access to PMI information through a port rather than just an fd is allowed])
    else
        AC_MSG_ERROR([Cannot build simple PMI with support for an IP port because of missing functions])
    fi
fi

# Check for socklen_t .  If undefined, define it as int
# (note the conditional inclusion of sys/socket.h)
AC_CACHE_CHECK([whether socklen_t is defined (in sys/socket.h if present)], pac_cv_have_socklen_t,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <sys/types.h>
        #ifdef HAVE_SYS_SOCKET_H
        #include <sys/socket.h>
        #endif

        typedef struct { double a; int b; } socklen_t;
        ]],[[
        socklen_t a;a.a=1.0;
        ]])],pac_cv_have_socklen_t=no,pac_cv_have_socklen_t=yes)
])
if test "$pac_cv_have_socklen_t" = no ; then
    AC_DEFINE(socklen_t,int,[Define if socklen_t is not defined])
fi
# Check for h_addr or h_addr_list
AC_CACHE_CHECK([whether struct hostent contains h_addr_list], pac_cv_have_haddr_list,[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]],[[struct hostent hp;hp.h_addr_list[0]=0;]])],
        pac_cv_have_haddr_list=yes,pac_cv_have_haddr_list=no)
])
if test "$pac_cv_have_haddr_list" = "yes" ; then
    AC_DEFINE(HAVE_H_ADDR_LIST,1,[Define if struct hostent contains h_addr_list])
fi
PAC_C_GNU_ATTRIBUTE

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
