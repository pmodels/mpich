[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_CRAY],[test "x$pmi_name" = "xcraypmi"])
AM_COND_IF([BUILD_PMI_CRAY],[

# set CPPFLAGS and LDFLAGS
PAC_PREPEND_FLAG([$CRAY_PMI_INCLUDE_OPTS], [CPPFLAGS])
PAC_PREPEND_FLAG([$CRAY_PMI_POST_LINK_OPTS], [LDFLAGS])

AC_CHECK_HEADER([pmi.h], [], [AC_MSG_ERROR([could not find pmi.h.  Configure aborted])])
AC_CHECK_LIB([pmi], [PMI_Init], [], [AC_MSG_ERROR([could not find the cray libpmi.  Configure aborted])])

AC_DEFINE(USE_CRAYPMI_API, 1, [Define if Cray PMI API must be used])
PAC_APPEND_FLAG([-lpmi], [WRAPPER_LIBS])

])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
