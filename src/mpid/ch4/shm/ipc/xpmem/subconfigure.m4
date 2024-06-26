[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        build_ch4_shm_ipc_xpmem=auto
    ])dnl end AM_COND_IF(BUILD_CH4,...)
])dnl end _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    if test -n "$build_ch4_shm_ipc_xpmem" ; then
        AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:xpmem])
        PAC_CHECK_HEADER_LIB_OPTIONAL([xpmem],[xpmem.h],[xpmem],[xpmem_make])
        if test "$pac_have_xpmem" = "yes" ; then
            AC_DEFINE(MPIDI_CH4_SHM_ENABLE_XPMEM, 1, [Enable XPMEM shared memory submodule in CH4])
            if test "${build_ch4_shm_ipc_xpmem}" = "auto" ; then
                AC_DEFINE([MPIDI_CH4_SHM_XPMEM_ALLOW_SILENT_FALLBACK],[1],
                          [Silently disable XPMEM, if it fails at runtime])
            fi
        fi
    fi
    AM_CONDITIONAL([BUILD_SHM_IPC_XPMEM],[test "$pac_have_xpmem" = "yes"])
])dnl end _BODY

[#] end of __file__
