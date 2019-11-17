[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[xpmem],[build_ch4_shm_xpmem=yes])
        done
    ])dnl end AM_COND_IF(BUILD_CH4,...)

    AM_CONDITIONAL([BUILD_SHM_XPMEM],[test "X$build_ch4_shm_xpmem" = "Xyes"])
])dnl end _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_SHM_XPMEM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:xpmem])

    PAC_CHECK_HEADER_LIB_FATAL([xpmem],[xpmem.h],[xpmem],[xpmem_make])
    if test "${have_xpmem}" = "yes" ; then
        PAC_APPEND_FLAG([-lxpmem],[WRAPPER_LIBS])
        AC_DEFINE(MPIDI_CH4_SHM_ENABLE_XPMEM, 1, [Enable XPMEM shared memory submodule in CH4])
    fi
    AM_CONDITIONAL([BUILD_SHM_XPMEM],[test "$have_xpmem" = "yes"])

])dnl end AM_COND_IF(BUILD_SHM_XPMEM,...)
])dnl end _BODY

[#] end of __file__
