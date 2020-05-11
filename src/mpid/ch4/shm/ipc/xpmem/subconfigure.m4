[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[xpmem],[build_ch4_shm_ipc_xpmem=yes])
        done
    ])dnl end AM_COND_IF(BUILD_CH4,...)

    AM_CONDITIONAL([BUILD_SHM_IPC_XPMEM],[test "X$build_ch4_shm_ipc_xpmem" = "Xyes"])
])dnl end _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_SHM_IPC_XPMEM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:xpmem])

        PAC_SET_HEADER_LIB_PATH(xpmem)
        PAC_PUSH_FLAG(LIBS)
        PAC_CHECK_HEADER_LIB([xpmem.h],[xpmem],[xpmem_make],[have_xpmem=yes],[have_xpmem=no])
        if test "${have_xpmem}" = "yes" ; then
            PAC_APPEND_FLAG([-lxpmem],[WRAPPER_LIBS])
        else
            AC_MSG_ERROR(['xpmem.h or libxpmem library not found.'])
        fi
        PAC_POP_FLAG(LIBS)
        AM_CONDITIONAL([BUILD_SHM_IPC_XPMEM],[test "$have_xpmem" = "yes"])

        if test "${have_xpmem}" = "yes" ; then
            AC_DEFINE(MPIDI_CH4_SHM_ENABLE_XPMEM, 1, [Enable XPMEM shared memory submodule in CH4])
        fi
])dnl end AM_COND_IF(BUILD_SHM_IPC_XPMEM,...)
])dnl end _BODY

[#] end of __file__
