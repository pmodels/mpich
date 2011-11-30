[#] start of __file__
dnl MPICH2_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[psm],[build_nemesis_netmod_psm=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_PSM],[test "X$build_nemesis_netmod_psm" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_PSM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:psm])
    PAC_CHECK_HEADER_LIB_FATAL([psm], [psm.h], [psm_infinipath], [psm_init])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_PSM,...)
])dnl end _BODY
[#] end of __file__
