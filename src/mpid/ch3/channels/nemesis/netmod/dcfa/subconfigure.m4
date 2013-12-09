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

    PAC_CHECK_HEADER_LIB(dcfa.h,dcfa,ibv_open_device,dcfa_found=yes,dcfa_found=no)
    if test "${dcfa_found}" = "yes" ; then
        AC_MSG_NOTICE([libdcfa is going to be linked.])
    else
        PAC_CHECK_HEADER_LIB([infiniband/verbs.h],ibverbs,ibv_open_device,ibverbs_found=yes,ibverbs_found=no)
        if test "${ibverbs_found}" = "yes" ; then
            AC_MSG_NOTICE([libibverbs is going to be linked.])
        else
            AC_MSG_ERROR([Internal error: neither ibverbs nor dcfa was found])
        fi
    fi

    AC_DEFINE([MPID_NEM_DCFA_VERSION], ["0.9.0"], [Version of netmod/DCFA])
    AC_DEFINE([MPID_NEM_DCFA_RELEASE_DATE], ["2013-11-18"], [Release date of netmod/DCFA])
    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions, registered in dcfa_init.c])
#    AC_DEFINE([ENABLE_RNDV_WAIT_TIMER], 1, [make MPI_Wtime returns wait time. Wait time is elapsed time from MPIDI_CH3_Progress_start to MPIDI_CH3_Progress_end])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_DCFA,...)
])dnl end _BODY

[#] end of __file__
