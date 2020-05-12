[#] start of __file__

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_CONDITIONAL([BUILD_MPID_COMMON_SHM],[test "X$build_mpid_common_shm" = "Xyes"])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

gl_FUNC_RANDOM_R
if test "$HAVE_RANDOM_R" = "1" -a "$HAVE_STRUCT_RANDOM_DATA" = "1" ; then
    AC_DEFINE(USE_SYM_HEAP,1,[Define if we can use a symmetric heap])
    AC_MSG_NOTICE([Using a symmetric heap])
else
    AC_MSG_NOTICE([Using a non-symmetric heap])
fi

dnl the code below is currently commented out because the common shm code
dnl doesn't need to run any configure tests, it really just needs to emit the
dnl AM_CONDITIONAL for the moment
dnl
dnl AM_COND_IF([BUILD_MPID_COMMON_SHM],[
dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/mpid/common/shm])
dnl 
dnl ])dnl end AM_COND_IF(BUILD_MPID_COMMON_SHM,...)
])dnl end _BODY
dnl
[#] end of __file__
