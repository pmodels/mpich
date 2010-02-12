dnl Process this file with autoconf to produce a configure script.
dnl
dnl aclocal_cache.m4, included by sowing/confdb/aclocal.m4, fixes 
dnl bugs in autoconf caching.
dnl
dnl The file name here refers to a file in the source being configured
AC_INIT(simple2pmi.c)

dnl Definitions will be placed in this file rather than in the DEFS variable
AC_CONFIG_HEADER(pmi2conf.h)
AH_TOP([/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef PMI2CONF_H_INCLUDED
#define PMI2CONF_H_INCLUDED
])
AH_BOTTOM([#endif])
dnl
dnl Set the directory that contains support scripts such as install-sh and
dnl config.guess
dnl AC_CONFIG_AUX_DIR(../../../confdb)
dnl
echo "RUNNING CONFIGURE FOR THE SIMPLE PMI VERSION 2"
dnl
dnl
dnl Use AC_ARG_ENABLE to look for --enable-feature and AC_ARG_WITH to look for
dnl --with-capability
dnl
dnl Enable better caching control
PAC_ARG_CACHING

AC_ARG_ENABLE(pmiport,
[--enable-pmiport - Allow PMI interface to use a host-port pair to contact
                   for PMI services],,enable_pmiport=default)
if test "$enable_pmiport" != "no" ; then
   enable_pmiport=yes
fi

PAC_VPATH_CHECK()

PAC_LOAD_BASE_CACHE

PAC_PROG_MAKE

# Check for alloca function.  May set HAVE_ALLOCA_H and HAVE_ALLOCA
AC_FUNC_ALLOCA
# We don't use alloca unless USE_ALLOCA is also set.
AC_ARG_ENABLE(alloca,
[--enable-alloca - Use alloca to allocate temporary memory if available],,enable_alloca=no)
if test "$enable_alloca" = yes ; then
    AC_DEFINE(USE_ALLOCA,1,[Define if alloca should be used if available])
fi

AC_CHECK_HEADERS(unistd.h string.h stdlib.h sys/socket.h strings.h)
dnl Use snprintf if possible when creating messages
AC_CHECK_FUNCS(snprintf)
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi
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

AC_CHECK_SIZEOF(void *)


AC_SUBST(AR)
AC_SUBST(AR_FLAGS)
AC_SUBST(RANLIB)
AC_SUBST(MPILIBNAME)
AC_SUBST(CC)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
PAC_CC_SUBDIR_SHLIBS
AC_SUBST(master_top_srcdir)
AC_SUBST(master_top_builddir)
dnl
dnl Dependency handling
AC_SUBST(MAKE_DEPEND_C)
dnl
dnl Etags
AC_SUBST(ETAGS)
AC_SUBST(ETAGSADD)
export ETAGS
export ETAGSADD

PAC_UPDATE_BASE_CACHE

# Make sure that the dependencies directories are present
AC_OUTPUT_COMMANDS([if test ! -d .deps ; then mkdir .deps ; fi 
if test ! -f .deps/alldeps ; then touch .deps/alldeps ; fi])

AC_OUTPUT(Makefile localdefs)

PAC_SUBDIR_CACHE_CLEANUP
