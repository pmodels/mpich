[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/pm/util

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_COND_IF([BUILD_PM_GFORKER],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/pm/gforker])

# tell src/pm/util to configure itself
build_pm_util=yes

AC_ARG_ENABLE([allowport],
              [AS_HELP_STRING([--enable-allowport],
                              [Turn on use of a port for communicating with the processes started by mpiexec])],
              [],
              [enable_allowport=yes])

dnl
dnl Check for special compile characteristics
dnl
dnl Is there libnsl needed for gethostbyname?
dnl AC_SEARCH_LIBS(gethostbyname,nsl)
AC_SEARCH_LIBS([socketpair],[socket])
dnl
dnl Check for a specific header
AC_CHECK_HEADERS([sys/types.h signal.h sys/ptrace.h])
dnl
dnl Check for functions.  This invokes another test if the function is 
dnl found.  The braces around the second test are essential. 
dnl AC_CHECK_FUNC(setpgrp,[AC_FUNC_SETPGRP])
AC_CHECK_FUNCS([strsignal])
dnl 
dnl Check for signal handlers
AC_CHECK_FUNCS([sigaction signal sigset])
sigaction_ok=no
if test "$ac_cv_func_sigaction" = "yes" ; then
    AC_CACHE_CHECK([for struct sigaction],pac_cv_struct_sigaction,[
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
            #include <signal.h>
            ]],[[
            struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
            ]])], pac_cv_struct_sigaction="yes",pac_cv_struct_sigaction="no")
    ])
    if test "$pac_cv_struct_sigaction" = "no" ; then
        AC_CACHE_CHECK([for struct sigaction with _POSIX_SOURCE], pac_cv_struct_sigaction_needs_posix,[
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                #define _POSIX_SOURCE
                #include <signal.h>
                ]],[[
                struct sigaction act; sigaddset( &act.sa_mask, SIGINT );
                ]])],
                pac_cv_struct_sigaction_needs_posix="yes",
                pac_cv_struct_sigaction_needs_posix="no")
        ])
        if test "$pac_cv_struct_sigaction_needs_posix" = "yes" ; then
            sigaction_ok=yes
	fi
    else
        sigaction_ok=yes
    fi
fi
dnl
# Decide on the signal handler to use
if test "$ac_cv_func_sigaction" = "yes" -a "$sigaction_ok" = "yes" ; then
    # FIXME DJG: where should this get set?
    if test "$pac_struct_sigaction_needs_posix" = yes ; then
        AC_DEFINE(NEEDS_POSIX_FOR_SIGACTION,1,[Define if _POSIX_SOURCE needed to get sigaction])
    fi
    AC_DEFINE(USE_SIGACTION,1,[Define if sigaction should be used to set signals])
elif test "$ac_cv_func_signal" = "yes" ; then
    AC_DEFINE(USE_SIGNAL,1,[Define if signal should be used to set signals])
fi
dnl
if test "$enable_allowport" = "yes" ; then
   AC_DEFINE(MPIEXEC_ALLOW_PORT,1,[Define if a port may be used to communicate with the processes])
fi

# some of these may be redundant with the upper level code, although the caching
# should detect it and make the performance impact a non-issue
AC_CHECK_HEADERS([string.h sys/time.h unistd.h stdlib.h sys/socket.h wait.h errno.h])

])dnl end AM_COND_IF(BUILD_PM_GFORKER,...)
])dnl end _BODY
dnl
[#] end of __file__
