[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    PAC_CHECK_HEADER_LIB_OPTIONAL(hcoll,[hcoll/api/hcoll_api.h],[hcoll],[hcoll_init])
    if test "$pac_have_hcoll" = "yes" ; then
        AC_DEFINE([HAVE_HCOLL],1,[Define if building hcoll])
    fi
    AM_CONDITIONAL([BUILD_HCOLL],[test "$pac_have_hcoll" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# nothing to do
])dnl end _BODY

[#] end of __file__
