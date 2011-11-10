[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_src_pm_smpd],[
    AC_ARG_WITH([smpd-sock],
                [AS_HELP_STRING([--with-smpd-sock=SOCK_IMPL],[selects a socket implementation for SMPD])],
                [],
                [with_smpd_sock=poll])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_src_pm_smpd],[

# the pm_names variable is set by the top level configure
build_smpd=no
for pm_name in $pm_names ; do
    if test "X$pm_name" = "Xsmpd" ; then
        build_smpd=yes
    fi
done
AM_CONDITIONAL([BUILD_PM_SMPD],[test "X$build_smpd" = "Xyes"])

# first_pm_name is set by the top level configure
AM_CONDITIONAL([PRIMARY_PM_SMPD],[test "X$first_pm_name" = "Xsmpd"])

AM_COND_IF([BUILD_PM_SMPD],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/pm/smpd])

# Check that we are using the simple PMI implementation
# (Selecting multiple PMs may require incompatible PMI implementations 
# (e.g., gforker and SMPD).
if test -z "$PM_REQUIRES_PMI" ; then
    PM_REQUIRES_PMI=smpd
elif test "$PM_REQUIRES_PMI" != "smpd" ; then
    echo "smpd requires the smpd PMI implementation; $PM_REQUIRES_PMI has already been selected"
    exit 1
fi

# a lot of checks that will mostly be redundant with the top level code, but
# caching will avoid any real speed penalty
AC_CHECK_HEADERS([sys/types.h sys/wait.h signal.h unistd.h stdlib.h errno.h stdarg.h pthread.h sys/socket.h sys/select.h sys/stat.h ctype.h])
AC_CHECK_HEADERS([math.h stdio.h termios.h string.h sys/stat.h malloc.h uuid/uuid.h])
AC_CHECK_HEADERS([stddef.h md5.h openssl/md5.h])
AC_CHECK_FUNCS([sigaction usleep CFUUIDCreate uuid_generate setenv md5_calc])
AC_SEARCH_LIBS([uuid_generate], [uuid])

AC_MSG_CHECKING([for poll stdin capability])
AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd;
    struct pollfd a[1];
    int result;
    int flags;

    fd = fileno(stdin);
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
	return -1;
    }
    flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (flags == -1)
    {
	return -1;
    }

    a[0].fd = fd;
    a[0].events = POLLIN;
    a[0].revents = 0;

    result = poll(a, 1, 1);
    if (result == 1)
    {
	if (a[0].revents & POLLNVAL)
	{
	    return 0;
	}
    }
    return -1;
}
], AC_MSG_RESULT([no])
   AC_DEFINE([USE_PTHREAD_STDIN_REDIRECTION],[1],[define if the file descriptor for stdin cannot be passed to poll]), 
   AC_MSG_RESULT([yes]),
   AC_MSG_RESULT([not checking when cross compiling]))
AC_SEARCH_LIBS([MD5], [ssl crypto])
# Make configure check for MD5 so that the variable will be set
AC_CHECK_FUNCS([MD5])
#
AC_SEARCH_LIBS([md5_calc], [md5])
AC_MSG_CHECKING([for md5_calc function])
AC_TRY_COMPILE([
#ifdef HAVE_STDDEF_H
 #include <stddef.h>
#endif
#include <md5.h>
],[unsigned char hash[16];
unsigned char c='a';
md5_calc(hash,&c,1);],[ac_cv_have_md5_calc=yes],[ac_cv_have_md5_calc=no])
if test "$ac_cv_have_md5_calc" = yes ; then
    AC_DEFINE([HAVE_MD5_CALC],[1],[Define if you have the md5_calc function.])
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# md5 appears to be required for SMPD.  Test that we have either mp5_calc
# or MP5 (may be part of openssl/md5.h)
if test "$ac_cv_have_md5_calc" != yes -a \
  \( "$ac_cv_header_openssl_md5_h" != yes -o "$ac_cv_func_MD5" != yes \) ; then
    AC_MSG_ERROR([SMPD requires MD5 support, and configure could not find either md5_calc in md5.h or MD5 in openssl/md5.h])
fi

])dnl end AM_COND_IF(BUILD_PM_SMPD,...)
])dnl end _BODY
dnl
[#] end of __file__
