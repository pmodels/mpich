[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_MPID_COMMON_SCHED],[test "X$build_mpid_common_sched" = "Xyes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

dnl the code below is currently commented out because the common sched code
dnl doesn't need to run any configure tests, it really just needs to emit the
dnl AM_CONDITIONAL for the moment
dnl
dnl AM_COND_IF([BUILD_MPID_COMMON_SCHED],[
dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/mpid/common/sched])
dnl 
dnl ])dnl end AM_COND_IF(BUILD_MPID_COMMON_SCHED,...)
])dnl end _BODY
dnl
[#] end of __file__
