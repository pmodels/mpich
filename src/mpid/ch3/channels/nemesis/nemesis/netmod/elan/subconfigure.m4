[#] start of __file__
dnl MPICH2_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[elan],[build_nemesis_netmod_elan=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_ELAN],[test "X$build_nemesis_netmod_elan" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_ELAN],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:elan])
    AC_MSG_WARN([=== You're about to use the experimental Nemesis/Elan network module.])
    AC_MSG_WARN([=== This module has not been thoroughly tested and some performance issues remain.])
    PAC_CHECK_HEADER_LIB_FATAL([elan], [elan/elan.h], [elan], [elan_baseInit])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_ELAN,...)
])dnl end _BODY
[#] end of __file__
