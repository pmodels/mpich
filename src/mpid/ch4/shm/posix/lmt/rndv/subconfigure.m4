[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for lmt in $ch4_posix_lmt_modules ; do
            AS_CASE([$lmt],[rndv],[build_ch4_shm_posix_lmt_rndv=yes])
            if test $lmt = "rndv" ; then
                AC_DEFINE(HAVE_CH4_SHM_LMT_RNDV,1,[RNDV submodule is built])
            fi
        done
    ])
    AM_CONDITIONAL([BUILD_CH4_SHM_POSIX_LMT_RNDV],[test "X$build_ch4_shm_posix_lmt_rndv" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
])dnl end _BODY

[#] end of __file__

