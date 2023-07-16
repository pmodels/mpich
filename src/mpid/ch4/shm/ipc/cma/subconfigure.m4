[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AC_ARG_ENABLE(cma,
	AS_HELP_STRING([--disable-cma], [Disable CMA. The default is enable=yes.]),
	[], [enable_cma=yes])
])dnl end _PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        if test "$enable_cma" = "yes"; then
            AC_CHECK_HEADERS([sys/uio.h])
            AC_CHECK_FUNCS([process_vm_readv])
            if test "$ac_cv_func_process_vm_readv" = "yes" ; then
                AC_DEFINE(MPIDI_CH4_SHM_ENABLE_CMA, 1, [Enable CMA IPC in CH4])
            fi
        fi
    ])dnl end AM_COND_IF(BUILD_CH4,...)
])dnl end _BODY

[#] end of __file__
