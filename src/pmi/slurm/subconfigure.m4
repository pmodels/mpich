[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
if test "x$pmi_name" = "xslurm" ; then
    # sets CPPFLAGS and LDFLAGS
    PAC_SET_HEADER_LIB_PATH([slurm])

    AC_CHECK_HEADER([slurm/pmi.h], [], [AC_MSG_ERROR([could not find slurm/pmi.h.  Configure aborted])])
    AC_CHECK_LIB([pmi], [PMI_Init],
                 [PAC_PREPEND_FLAG([-lpmi],[LIBS])
                  PAC_PREPEND_FLAG([-lpmi], [WRAPPER_LIBS])],
                 [AC_MSG_ERROR([could not find the Slurm libpmi.  Configure aborted])])
fi
])dnl end BODY macro

[#] end of __file__
