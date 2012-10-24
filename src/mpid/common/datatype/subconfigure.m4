[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_MPID_COMMON_DATATYPE],[test "X$build_mpid_common_datatype" = "Xyes"])
])


dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# FIXME XXX DJG TODO move configure tests for datatype/dataloop code here?
dnl the code below is currently commented out because the common datatype code
dnl doesn't need to run any configure tests, it really just needs to emit the
dnl AM_CONDITIONAL for the moment
dnl
dnl AM_COND_IF([BUILD_MPID_COMMON_DATATYPE],[
dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/mpid/common/datatype])
dnl 
dnl ])dnl end AM_COND_IF(BUILD_MPID_COMMON_DATATYPE,...)
])dnl end _BODY
dnl
[#] end of __file__
