[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_SLURM],[test "x$pmi_name" = "xslurm"])
AM_COND_IF([BUILD_PMI_SLURM],[

# sets CPPFLAGS and LDFLAGS
PAC_SET_HEADER_LIB_PATH([slurm])

AC_CHECK_HEADER([slurm/pmi.h], [], [AC_MSG_ERROR([could not find slurm/pmi.h.  Configure aborted])])
AC_CHECK_LIB([pmi], [PMI_Init],
             [PAC_PREPEND_FLAG([-lpmi],[LIBS])
              PAC_PREPEND_FLAG([-lpmi], [WRAPPER_LIBS])],
             [AC_MSG_ERROR([could not find the slurm libpmi.  Configure aborted])])

])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
