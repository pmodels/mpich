[#] start of __file__
dnl MPICH2_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[portals4nm],[build_nemesis_netmod_portals4nm=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_PORTALS4NM],[test "X$build_nemesis_netmod_portals4nm" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_PORTALS4NM],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:portals4nm])

    PAC_SET_HEADER_LIB_PATH(portals4)
    PAC_CHECK_HEADER_LIB_FATAL(portals4, portals4.h, portals, PtlInit)

])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_PORTALS4NM,...)
])dnl end _BODY

[#] end of __file__
