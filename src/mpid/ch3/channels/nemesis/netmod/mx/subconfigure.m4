[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[mx],[build_nemesis_netmod_mx=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_MX],[test "X$build_nemesis_netmod_mx" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_MX],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:mx])

    PAC_SET_HEADER_LIB_PATH(mx)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB_FATAL(mx, myriexpress.h, myriexpress, mx_finalize)
    PAC_POP_FLAG(LIBS)
    AC_CHECK_HEADER([mx_extensions.h], , [
             AC_MSG_ERROR(['mx_extensions.h not found. Are you running a recent version of MX (at least 1.2.1)?'])
     ])
     AC_TRY_COMPILE([
     #include "myriexpress.h"
     #include "mx_extensions.h"
     #if MX_API < 0x301             
     #error You need at least MX 1.2.1 (MX_API >= 0x301)
     #endif],
     [int a=0;],
     mx_api_version=yes,
     mx_api_version=no)
     if test "$mx_api_version" = no ; then
        AC_MSG_ERROR(['MX API version Problem.  Are you running a recent version of MX (at least 1.2.1)?'])
     fi;
     AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions])
     PAC_APPEND_FLAG([-lmx],[EXTERNAL_LIBS])

])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_MX,...)
])dnl end _BODY
[#] end of __file__
