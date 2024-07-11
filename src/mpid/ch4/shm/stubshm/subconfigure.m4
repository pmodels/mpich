[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        build_ch4_shm_stubshm=no
    ])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    if test -n "$build_ch4_shm_ipc_stubshm" ; then
        AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:stubshm])
    fi
    AM_CONDITIONAL([BUILD_SHM_STUBSHM],[test "X$build_ch4_shm_stubshm" = "Xyes"])
])dnl end _BODY

[#] end of __file__
