[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_MPID_COMMON_BC],[test "$build_mpid_common_bc" = "yes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[])
[#] end of __file__
