[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        build_ch4_shm_ipc_gpu=auto
    ])dnl end of AM_COND_IF(BUILD_CH4,...)
])dnl end of _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    # GPU_SUPPORT is set by mpl via src/mpl/localdefs.in
    AM_CONDITIONAL([BUILD_SHM_IPC_GPU],[test -n "$GPU_SUPPORT"])
    AM_COND_IF([BUILD_SHM_IPC_GPU],[
        AC_DEFINE([MPIDI_CH4_SHM_ENABLE_GPU],[1],[Define if GPU IPC submodule is enabled])
    ])
])dnl end of _BODY

[#] end of __file__
