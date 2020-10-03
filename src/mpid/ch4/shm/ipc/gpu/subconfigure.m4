[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_IF([test "x$shm" = "xgpudirect"],[build_ch4_shm_ipc_gpu=yes],
                  [test "x$shm" = "xauto"],[build_ch4_shm_ipc_gpu=yes])
        done

        if test "$build_ch4_shm_ipc_gpu" = "yes" ; then
            AC_DEFINE([MPIDI_CH4_SHM_ENABLE_GPU],[1],[Define if GPU IPC submodule is enabled])
        fi
    ])dnl end of AM_COND_IF(BUILD_CH4,...)

    AM_CONDITIONAL([BUILD_SHM_IPC_GPU],[test "X$build_ch4_shm_ipc_gpu" = "Xyes"])
])dnl end of _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
])dnl end of _BODY

[#] end of __file__
