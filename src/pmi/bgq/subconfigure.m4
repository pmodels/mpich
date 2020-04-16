[#] start of __file__

[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_CONDITIONAL([BUILD_PMI_BGQ],[test "x$pmi_name" = "xbgq"])
AM_COND_IF([BUILD_PMI_BGQ],[

# This is a hack to include the pmi.h header. The OFI/BGQ provider
# includes PMI functions, but no header file.
PAC_PREPEND_FLAG([-I${use_top_srcdir}/src/pmi/include], [CPPFLAGS])

])dnl end COND_IF

])dnl end BODY macro

m4_define([PAC_SRC_PMI_BGQ_SUBCFG_MODULE_LIST],
[src_pmi_bgq])

[#] end of __file__
