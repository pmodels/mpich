[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/pmi

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_PMI2_POE],[test "x$pmi_name" = "xpmi2/poe"])

AM_COND_IF([BUILD_PMI_PMI2_POE],[
if test "$enable_pmiport" != "no" ; then
   enable_pmiport=yes
fi

dnl causes USE_PMI2_API to be AC_DEFINE'ed by the top-level configure.ac
USE_PMI2_API=yes

PAC_C_GNU_ATTRIBUTE
])dnl end COND_IF

])dnl end BODY macro

[#] end of __file__
