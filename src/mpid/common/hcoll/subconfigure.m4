[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
	AC_ARG_ENABLE(hcoll,
	    AC_HELP_STRING([--enable-hcoll], [Enable HCOLL collectives support.]))

	PAC_SET_HEADER_LIB_PATH(hcoll)
	PAC_PUSH_FLAG(LIBS)
	if test "$enable_hcoll" != "no"; then
	   PAC_CHECK_HEADER_LIB([hcoll/api/hcoll_api.h],[hcoll],[hcoll_init],[have_hcoll=yes],[have_hcoll=no])
	   if test "$have_hcoll" = "yes"; then
	       PAC_APPEND_FLAG([-lhcoll],[WRAPPER_LIBS])
	   elif test ! -z "${with_hcoll}" ; then
	       AC_MSG_ERROR(['hcoll/api/hcoll_api.h or libhcoll library not found.'])
	   elif test "$enable_hcoll" = "yes"; then
	       AC_MSG_ERROR(['hcoll was selected for build, but could not be configured.'])
	   fi
	fi
	PAC_POP_FLAG(LIBS)
	AM_CONDITIONAL([BUILD_HCOLL],[test "$have_hcoll" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# nothing to do
])dnl end _BODY

[#] end of __file__
