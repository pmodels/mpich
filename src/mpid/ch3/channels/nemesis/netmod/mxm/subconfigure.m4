[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[mxm],[build_nemesis_netmod_mxm=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_MXM],[test "X$build_nemesis_netmod_mxm" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_MXM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:mxm])

    PAC_SET_HEADER_LIB_PATH(mxm)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB_FATAL(mxm, [mxm/api/mxm_api.h], mxm, mxm_get_version)
    PAC_POP_FLAG(LIBS)
    AC_CHECK_HEADER([mxm/api/mxm_api.h], , [
             AC_MSG_ERROR(['mxm/api/mxm_api.h not found.'])
     ])
     AC_TRY_COMPILE([
     #include "mxm/api/mxm_version.h"
#ifndef MXM_VERSION
#error "MXM Version is less than 1.5, please upgrade"
#endif
#
#if MXM_API < MXM_VERSION(3,1)
#error "MXM Version is less than 3.1, please upgrade"
#endif],
     [int a=0;],
     mxm_api_version=yes,
     mxm_api_version=no)
     if test "$mxm_api_version" = no ; then
        AC_MSG_ERROR(['MXM API version Problem.  Are you running a recent version of MXM (at least 3.1)?'])
     fi;
     AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions])
     PAC_APPEND_FLAG([-lmxm],[WRAPPER_LIBS])

])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_MXM,...)
])dnl end _BODY
[#] end of __file__
