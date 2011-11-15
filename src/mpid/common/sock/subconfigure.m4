[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_src_mpid_common_sock],[
    AM_CONDITIONAL([BUILD_MPID_COMMON_SOCK],[test "X$build_mpid_common_sock" = "Xyes"])
    AM_COND_IF([BUILD_MPID_COMMON_SOCK],[
        # we unconditionally enable the "poll" implementation because we don't
        # use this build system on windows right now
        build_mpid_common_sock_poll=yes
    ])
])

dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_src_mpid_common_sock],[
AM_COND_IF([BUILD_MPID_COMMON_SOCK],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR src/mpid/common/sock])

# Adding this prevents the pesky "(strerror() not found)" problem, which can be
# very frustrating in the sock code, the most likely to receive an error code
# from the system. [goodell@ 2008-01-10]
AC_CHECK_FUNCS([strerror])

])dnl end AM_COND_IF(BUILD_MPID_COMMON_SOCK,...)
])dnl end _BODY
dnl
[#] end of __file__
