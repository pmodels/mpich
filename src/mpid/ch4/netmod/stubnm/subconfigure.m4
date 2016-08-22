[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for net in $ch4_netmods ; do
            AS_CASE([$net],[stubnm],[build_ch4_netmod_stubnm=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_CH4_NETMOD_STUBNM],[test "X$build_ch4_netmod_stubnm" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_STUBNM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:stubnm])
    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions])
    AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
])dnl end AM_COND_IF(BUILD_CH4_NETMOD_STUBNM,...)
])dnl end _BODY

[#] end of __file__
