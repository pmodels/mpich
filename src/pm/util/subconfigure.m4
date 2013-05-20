[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

dnl
dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PM_UTIL],[test "X$build_pm_util" = "Xyes"])

AM_COND_IF([BUILD_PM_UTIL],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/pm/util])

AC_ARG_ENABLE([onsig],
              [AS_HELP_STRING([--enable-onsig],
                              [Control the handling of processes that signal (e.g., SEGV) using ptrace.  Disabled by default])],
              [],
              [enable_onsig=no])
AC_ARG_ENABLE([newsession],
              [AS_HELP_STRING([--enable-newsession],
                              [Create a new process group session if standard in is not connected to a terminal])],
              [],
              [enable_newsession=yes])

# Check for types
AC_TYPE_PID_T

if test "$enable_onsig" = "yes" ; then
    AC_CHECK_FUNCS(ptrace)
    # It isn't enough to find ptrace.  We also need the ptrace 
    # parameters, which some systems, such as IRIX, do not define.
    if test "$ac_cv_func_ptrace" = yes ; then
        AC_CACHE_CHECK([for ptrace named parameters],
pac_cv_has_ptrace_parms,[
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ptrace.h>],[int i = PTRACE_CONT;],pac_cv_has_ptrace_parms=yes,
pac_cv_has_ptrace_parms=no)])
        if test "$pac_cv_has_ptrace_parms" = "yes" ; then
            AC_DEFINE(HAVE_PTRACE_CONT,,[Define if ptrace parameters available])
        fi
    fi
fi

# Check for the functions needed to create a new session.
# Note that getsid may not have a prototype in unistd.h unless
# _XOPEN_SOURCE and _XOPEN_SOURCE_EXTENDED are defined, or
# if _XOPEN_SOURCE is defined as an integer 500 or larger (this
# for glibc).  The prototype should be
# pid_t getsid( pid_t pid );
#
# Cygwin has setsid but not getsid
AC_CHECK_FUNCS(setsid isatty getsid)
# See if we need to define getsid (in the case that the above XOPEN
# definitions have not been made.  
PAC_FUNC_NEEDS_DECL([#include <unistd.h>],getsid)
if test "$enable_newsession" = "yes" ; then
    AC_DEFINE(USE_NEW_SESSION,1,[Define if mpiexec should create a new process group session])
fi

# Check for convenient functions for the environment
AC_CHECK_FUNCS(unsetenv)

# Check for cygwin1.dll in /bin.  If found, define NEEDS_BIN_IN_PATH because
# we need to include bin in the path when spawning programs.
# This is the simplest possible test; lets hope that it is sufficient
AC_CACHE_CHECK([for cygwin1.dll in /bin],pac_cv_needs_bin_in_path,[
pac_cv_needs_bin_in_path=no
if test /bin/cygwin1.dll ; then 
    pac_cv_needs_bin_in_path=yes
fi])
if test "$pac_cv_needs_bin_in_path" = yes ; then
    AC_DEFINE(NEEDS_BIN_IN_PATH,1,[Define if /bin must be in path])
fi

# Look for alternatives.  Is environ in unistd.h?
AC_CACHE_CHECK([for environ in unistd.h],pac_cv_has_environ_in_unistd,[
AC_TRY_COMPILE([#include <unistd.h>],[char **ep = environ;],
pac_cv_has_environ_in_unistd=yes,pac_cv_has_environ_in_unistd=no)])

if test "$pac_cv_has_environ_in_unistd" != "yes" ; then
    # Can we declare it and use it?
    AC_CACHE_CHECK([for extern environ in runtime],
    pac_cv_has_extern_environ,[
    AC_TRY_LINK([extern char **environ;],[char **ep = environ;],
    pac_cv_has_extern_environ=yes,pac_cv_has_extern_environ=no)])
    if test "$pac_cv_has_extern_environ" = "yes" ; then
	AC_DEFINE(NEEDS_ENVIRON_DECL,1,[Define if environ decl needed] )
    fi
else
    pac_cv_has_extern_environ=yes
fi
if test "$pac_cv_has_extern_environ" = "yes" ; then
    AC_DEFINE(HAVE_EXTERN_ENVIRON,1,[Define if environ extern is available])
fi
dnl
dnl Check for special compile characteristics
dnl
dnl Is there libnsl needed for gethostbyname?
dnl AC_SEARCH_LIBS(gethostbyname,nsl)
AC_SEARCH_LIBS(socketpair,socket)
dnl
dnl Look for Standard headers
AC_HEADER_STDC
dnl Check for a specific header
AC_CHECK_HEADERS(sys/types.h signal.h sys/ptrace.h sys/uio.h unistd.h)
if test "$ac_cv_header_sys_uio_h" = "yes" ; then
    # Test for iovec defined
    AC_CACHE_CHECK([whether struct iovec is defined in sys/uio.h],
    pac_cv_has_struct_iovec,[
    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/uio.h>],[struct iovec v],pac_cv_has_struct_iovec=yes,
pac_cv_has_struct_iovec=no)])
    if test "$pac_cv_has_struct_iovec" = "yes" ; then
        AC_DEFINE(HAVE_IOVEC_DEFINITION,1,[Define if struct iovec defined in sys/uio.h])
    fi
fi
dnl
dnl Check for functions.  This invokes another test if the function is 
dnl found.  The braces around the second test are essential. 
dnl AC_CHECK_FUNC(setpgrp,[AC_FUNC_SETPGRP])
AC_CHECK_FUNCS(strsignal)
if test "$ac_cv_func_strsignal" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strsignal)
fi
AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(strdup)
dnl 
dnl Check for signal handlers
AC_CHECK_FUNCS(sigaction signal sigset)
sigaction_ok=no
if test "$ac_cv_func_sigaction" = "yes" ; then
    # Make sure that the fields that we need in sigaction are defined
    AC_CACHE_CHECK([for struct sigaction and sa_handler],
    pac_cv_struct_sigaction_with_sa_handler,[
    AC_TRY_COMPILE([#include <signal.h>],[
struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
act.sa_handler = SIG_IGN;],
    pac_cv_struct_sigaction_with_sa_handler=yes,
    pac_cv_struct_sigaction_with_sa_handler=no)])
    if test "$pac_cv_struct_sigaction_with_sa_handler" = "no" ; then
        AC_CACHE_CHECK([for struct sigaction and sa_handler with _POSIX_SOURCE],
	pac_cv_struct_sigaction_with_sa_handler_needs_posix,[
        AC_TRY_COMPILE([#define _POSIX_SOURCE
#include <signal.h>],[
struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
act.sa_handler = SIG_IGN;],
	pac_cv_struct_sigaction_with_sa_handler_needs_posix=yes,
	pac_cv_struct_sigaction_with_sa_handler_needs_posix=no)])
        if test "$pac_cv_struct_sigaction_with_sa_handler_needs_posix" = "yes" ; then
            sigaction_ok=yes
	fi
    else
        sigaction_ok=yes
    fi
fi
dnl
# Decide on the signal handler to use
if test "$ac_cv_func_sigaction" = "yes" -a "$sigaction_ok" = "yes" ; then
    if test "$pac_cv_struct_sigaction_with_sa_handler_needs_posix" = yes ; then
        AC_DEFINE(NEEDS_POSIX_FOR_SIGACTION,1,[Define if _POSIX_SOURCE needed to get sigaction])
    fi
    AC_DEFINE(USE_SIGACTION,,[Define if sigaction should be used to set signals])
elif test "$ac_cv_func_signal" = "yes" ; then
    AC_DEFINE(USE_SIGNAL,,[Define if signal should be used to set signals])
fi
dnl
# Check for needed declarations.  This must be after any step that might
# change the compilers behavior, such as the _POSIX_SOURCE test above
# FIXME: need to include the test, at least for any file that
# might set _POSIX_SOURCE
if test "$ac_cv_func_snprintf" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdio.h>],snprintf)
fi
if test "$ac_cv_func_strdup" = "yes" ; then
    # Do we need to declare strdup?
    PAC_FUNC_NEEDS_DECL([#include <string.h>],strdup)
fi

# putenv() sets environment variable
AC_HAVE_FUNCS(putenv)
if test "$ac_cv_func_putenv" = "yes" ; then
    PAC_FUNC_NEEDS_DECL([#include <stdlib.h>],putenv)
fi
# gethostname() returns host name
AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi


#
# Check for select and working FD_ZERO 
AC_CHECK_FUNCS(select)
AC_CHECK_HEADERS(sys/select.h)
if test "$ac_cv_func_select" != yes ; then
    AC_MSG_ERROR([select is required for the process manager utilities])
else
    # Check that FD_ZERO works.  Under the Darwin xlc (version 6) compiler,
    # FD_ZERO gets turned into a referece to __builtin_bzero, which is not
    # in the xlc libraries.  This is apparently due to xlc pretending that it
    # is GCC within the system header files (the same test that must 
    # succeed within the system header files to cause the declaration to
    # be __builtin_bzero fails outside of the header file).
    # (sys/select.h is POSIX)
    if test "$ac_cv_header_sys_select_h" = yes ; then
        AC_CACHE_CHECK([whether FD_ZERO works],pac_cv_fdzero_works,[
        AC_TRY_LINK([#include <sys/select.h>],[fd_set v; FD_ZERO(&v)],
        pac_cv_fdzero_works=yes,pac_cv_fdzero_works=no)])
        if test "$pac_cv_fdzero_works" != yes ; then
            AC_MSG_ERROR([Programs with FD_ZERO cannot be linked (check your system includes)])
	fi
    fi
fi
#
# Check for the Linux functions for controlling processor affinity.
# 
# LINUX: sched_setaffinity
# AIX:   bindprocessor
# OSX (Leopard): thread_policy_set
AC_CHECK_FUNCS([sched_setaffinity sched_getaffinity bindprocessor thread_policy_set])
if test "$ac_cv_func_sched_setaffinity" = "yes" ; then
    # Test for the cpu process set type
    AC_CACHE_CHECK([whether cpu_set_t available],pac_cv_have_cpu_set_t,[
    AC_TRY_COMPILE( [
#include <sched.h>],[ cpu_set_t t; ],pac_cv_have_cpu_set_t=yes,pac_cv_have_cpu_set_t=no)])
    if test "$pac_cv_have_cpu_set_t" = yes ; then
        AC_DEFINE(HAVE_CPU_SET_T,1,[Define if cpu_set_t is defined in sched.h])

	AC_CACHE_CHECK([whether the CPU_SET and CPU_ZERO macros are defined],
	pac_cv_cpu_set_defined,[
        AC_TRY_LINK( [
#include <sched.h>],[ cpu_set_t t; CPU_ZERO(&t); CPU_SET(1,&t); ],
        pac_cv_cpu_set_defined=yes,pac_cv_cpu_set_defined=no)])
	if test "$pac_cv_cpu_set_defined" = "yes" ; then
	    AC_DEFINE(HAVE_CPU_SET_MACROS,1,[Define if CPU_SET and CPU_ZERO defined])
        fi
	# FIXME: Some versions of sched_setaffinity return ENOSYS (!), 
	# so we should test for the unfriendly and useless behavior
    fi
fi
if test "$ac_cv_func_thread_policy_set" = yes ; then
    AC_CACHE_CHECK([whether thread affinity macros defined],
    pac_cv_have_thread_affinity_policy,[
    AC_TRY_COMPILE([#include <mach/thread_policy.h>],[
#if !defined(THREAD_AFFINITY_POLICY) || !defined(THREAD_AFFINITY_TAG_NULL)
    :'thread macros not defined
],[pac_cv_have_thread_affinity_policy=yes],
  [pac_cv_have_thread_affinity_policy=no])])
    if test "$pac_cv_have_thread_affinity_policy" = yes ; then
        AC_DEFINE(HAVE_OSX_THREAD_AFFINITY,1,[Define is the OSX thread affinity policy macros defined])
    fi
fi

AC_CHECK_HEADERS([string.h sys/time.h time.h stdlib.h sys/socket.h wait.h errno.h])
# Check for socklen_t .  
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
if test "$pac_cv_have_socklen_t" = yes ; then
    AC_DEFINE([HAVE_SOCKLEN_T],1,[Define if socklen_t is available])
fi

])dnl end AM_COND_IF(BUILD_PM_UTIL,...)
])dnl end _BODY
dnl
[#] end of __file__
