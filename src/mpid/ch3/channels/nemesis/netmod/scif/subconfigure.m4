[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[scif],[build_nemesis_netmod_scif=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_SCIF],[test "X$build_nemesis_netmod_scif" = "Xyes"])

    # check if getpagesize is available
    AC_CHECK_FUNCS(getpagesize)
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_SCIF],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:scif])
    PAC_SET_HEADER_LIB_PATH(scif)
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_SCIF,...)
])dnl end _BODY

[#] end of __file__
