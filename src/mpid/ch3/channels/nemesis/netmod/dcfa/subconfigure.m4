[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[dcfa],[build_nemesis_netmod_dcfa=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_DCFA],[test "X$build_nemesis_netmod_dcfa" = "Xyes"])

    # check if getpagesize is available
    AC_CHECK_FUNCS(getpagesize)
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_DCFA],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:dcfa])

    AC_ARG_ENABLE(dcfa, [--enable-dcfa - use DCFA library instead of IB Verbs library for MPICH/DCFA/McKernel/MIC],,enable_dcfa=no)
    if test "$enable_dcfa" = "yes" ; then
        AC_MSG_NOTICE([--enable-dcfa detected])
        PAC_CHECK_HEADER_LIB_FATAL(dcfa, dcfa.h, dcfa, ibv_open_device)
# see confdb/aclocal_libs.m4
    else   
        PAC_CHECK_HEADER_LIB_FATAL(ib, infiniband/verbs.h, ibverbs, ibv_open_device)
    fi                 

    AC_DEFINE([MPID_NEM_DCFA_VERSION], ["0.9.0"], [Version of netmod/DCFA])
    AC_DEFINE([MPID_NEM_DCFA_RELEASE_DATE], ["2013-11-18"], [Release date of netmod/DCFA])
    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions, registered in dcfa_init.c])
#    AC_DEFINE([ENABLE_RNDV_WAIT_TIMER], 1, [make MPI_Wtime returns wait time. Wait time is elapsed time from MPIDI_CH3_Progress_start to MPIDI_CH3_Progress_end])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_DCFA,...)
])dnl end _BODY

[#] end of __file__
