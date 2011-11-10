[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_src_pmi_smpd],[
])

AC_DEFUN([PAC_SUBCFG_BODY_src_pmi_smpd],[

AM_CONDITIONAL([BUILD_PMI_SMPD],[test "x$pmi_name" = "xsmpd"])

AM_COND_IF([BUILD_PMI_SMPD],[

AC_CHECK_HEADERS([stdarg.h unistd.h string.h stdlib.h dlfcn.h uuid/uuid.h mach-o/dyld.h ctype.h])
AC_CHECK_FUNCS([dlopen NSLinkModule])
AC_SEARCH_LIBS([dlopen], [dl])

AC_TRY_COMPILE([
#include <dlfcn.h>
],[int a;],[ac_cv_func_dlopen=yes],[ac_cv_func_dlopen=no])
if test "$ac_cv_func_dlopen" = yes ; then
    AC_DEFINE([HAVE_DLOPEN],[1],[Define if you have the dlopen function.])
fi

])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
