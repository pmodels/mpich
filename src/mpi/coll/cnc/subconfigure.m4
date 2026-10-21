[#] start of __file__

dnl Optional offload of selected collectives to an external libcncoll.
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    PAC_SET_HEADER_LIB_PATH(cnc)
    pac_have_cnc=no
    if test -n "${with_cnc}" -a "${with_cnc}" != "no" ; then
        AC_CHECK_HEADER([cnc/collective.h],[pac_have_cnc=yes],
                        [AC_MSG_ERROR([--with-cnc is given but cnc/collective.h was not found])])
    fi
    if test "$pac_have_cnc" = "yes" ; then
        PAC_LIBS_ADD(-lcncoll)
        AC_DEFINE([ENABLE_CNC],1,[Define if building with libcncoll])
    fi
    AM_CONDITIONAL([BUILD_CNC],[test "$pac_have_cnc" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
])dnl end BODY

[#] end of __file__
