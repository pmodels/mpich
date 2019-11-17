[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_SLURM],[test "x$pmi_name" = "xslurm"])
AM_COND_IF([BUILD_PMI_SLURM],[

    PAC_CHECK_HEADER_LIB_FATAL([slurm],[slurm/pmi.h],[pmi],[PMI_Init])
    if test $have_slurm = yes ; then
        PAC_PREPEND_FLAG([-lpmi], [WRAPPER_LIBS])
    fi

])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
