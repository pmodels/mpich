[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[pip],[build_ch4_shm_pip=yes])
        done
    ])dnl end AM_COND_IF(BUILD_CH4,...)

    AM_CONDITIONAL([BUILD_SHM_PIP],[test "X$build_ch4_shm_pip" = "Xyes"])
])dnl end _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_SHM_PIP],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:pip])
        AC_DEFINE(MPIDI_CH4_SHM_ENABLE_PIP, 1, [Enable PIP shared memory submodule in CH4])
        PAC_APPEND_FLAG([-lnuma],[LIBS])
])dnl end AM_COND_IF(BUILD_SHM_XPMEM,...)
])dnl end _BODY

[#] end of __file__
