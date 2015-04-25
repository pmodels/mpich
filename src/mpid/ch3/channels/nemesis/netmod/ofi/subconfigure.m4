[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[ofi],[build_nemesis_netmod_ofi=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_OFI],[test "X$build_nemesis_netmod_ofi" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_OFI],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:ofi])

    PAC_SET_HEADER_LIB_PATH(ofi)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB_FATAL(ofi, rdma/fabric.h, fabric, fi_getinfo)
    PAC_APPEND_FLAG([-lfabric],[WRAPPER_LIBS])
    PAC_POP_FLAG(LIBS)

    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_OFI,...)
])dnl end _BODY

[#] end of __file__
