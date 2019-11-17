[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    if test "${with_hcoll}" = "no" ; then
        have_hcoll=no;
    else
        PAC_CHECK_HEADER_LIB_ERROR([hcoll],[hcoll/api/hcoll_api.h],[hcoll],[hcoll_init])
        if test "${have_hcoll}" = "yes" ; then
            PAC_APPEND_FLAG([-lhcoll],[WRAPPER_LIBS])
        fi
    fi
    AM_CONDITIONAL([BUILD_HCOLL],[test "$have_hcoll" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# nothing to do
])dnl end _BODY

[#] end of __file__
