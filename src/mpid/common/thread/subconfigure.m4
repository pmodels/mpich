[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_MPID_COMMON_THREAD],[test "X$build_mpid_common_thread" = "Xyes"])
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_MPID_COMMON_THREAD],[
    dnl nothing to do for now...
    :
])dnl end AM_COND_IF(BUILD_MPID_COMMON_THREAD,...)
])dnl end _BODY

m4_define([PAC_SRC_MPID_COMMON_THREAD_SUBCFG_MODULE_LIST],
[src_mpid_common_thread])

[#] end of __file__
