[#] start of __file__
dnl MPICH2_SUBCFG_AFTER=src_mpid_ch3_channels_nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[gm],[build_nemesis_netmod_gm=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_GM],[test "X$build_nemesis_netmod_gm" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_GM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:gm])
    PAC_CHECK_HEADER_LIB_FATAL(gm, gm.h, gm, gm_init)
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_GM,...)
])dnl end _BODY
[#] end of __file__
