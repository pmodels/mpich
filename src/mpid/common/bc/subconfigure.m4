[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_MPID_COMMON_BC],[test "$build_mpid_common_bc" = "yes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[])

m4_define([PAC_SRC_MPID_COMMON_BC_SUBCFG_MODULE_LIST],
[src_mpid_common_bc])

[#] end of __file__
