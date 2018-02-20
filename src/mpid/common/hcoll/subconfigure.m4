[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
	PAC_SET_HEADER_LIB_PATH(hcoll)
	PAC_PUSH_FLAG(LIBS)
	PAC_CHECK_HEADER_LIB([hcoll/api/hcoll_api.h],[hcoll],[hcoll_init],[have_hcoll=yes],[have_hcoll=no])
	if test "${with_hcoll}" = "no" ; then
	   have_hcoll=no;
	elif test "${have_hcoll}" = "yes" ; then
	   PAC_APPEND_FLAG([-lhcoll],[WRAPPER_LIBS])
	elif test -n "${with_hcoll}" -o -n "${with_hcoll_lib}" -o -n "${with_hcoll_include}" ; then
	   AC_MSG_ERROR(['hcoll/api/hcoll_api.h or libhcoll library not found.'])
	fi
	PAC_POP_FLAG(LIBS)
	AM_CONDITIONAL([BUILD_HCOLL],[test "$have_hcoll" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# nothing to do
])dnl end _BODY

[#] end of __file__
