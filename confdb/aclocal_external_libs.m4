dnl **** libcr ************************************
AC_DEFUN([PAC_WITH_LIBCR], [
    PAC_SET_HEADER_LIB_PATH(blcr)
    PAC_PUSH_FLAG([LIBS])
    PAC_CHECK_HEADER_LIB_FATAL(blcr, libcr.h, cr, cr_init)
    PAC_POP_FLAG([LIBS])
    PAC_SUBDIR_SYSTEM([cr])
])

dnl **** libpmix ************************************
AC_DEFUN([PAC_WITH_PMIX], [
    PAC_SET_HEADER_LIB_PATH(pmix)
    if test -n "${with_pmix}" -a "${with_pmix}" != "no" ; then
        PAC_PUSH_FLAG([LIBS])
        PAC_CHECK_HEADER_LIB_FATAL(pmix, pmix.h, pmix, PMIx_Init)
        PAC_POP_FLAG([LIBS])
        PAC_SUBDIR_SYSTEM([pmix])
        with_pmix="yes"
    fi
])

