[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        build_ch4_shm_ipc_cma=auto
    ])dnl end AM_COND_IF(BUILD_CH4,...)
])dnl end _PREREQ


AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    if test -n "$build_ch4_shm_ipc_cma" ; then
        AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:cma])
        AC_CHECK_HEADERS([sys/uio.h])
        AC_CHECK_FUNCS([process_vm_readv])
        if test "$ac_cv_func_process_vm_readv" = "yes" ; then
            pac_have_cma=yes
            AC_DEFINE(MPIDI_CH4_SHM_ENABLE_CMA, 1, [Enable CMA IPC in CH4])
        fi
    fi
    AM_CONDITIONAL([BUILD_SHM_IPC_CMA],[test "$pac_have_cma" = "yes"])
])dnl end _BODY

[#] end of __file__
