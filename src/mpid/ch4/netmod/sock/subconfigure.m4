[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for net in $ch4_netmods ; do
            AS_CASE([$net],[sock],[build_ch4_netmod_sock=yes])
            if test $net = "sock" ; then
                AC_DEFINE(HAVE_CH4_NETMOD_SOCK,1,[SOCK netmod is built])
            fi
        done
    ])
    AM_CONDITIONAL([BUILD_CH4_NETMOD_SOCK],[test "X$build_ch4_netmod_sock" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_SOCK],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:sock])
])dnl end AM_COND_IF(BUILD_CH4_NETMOD_SOCK,...)
])dnl end _BODY

[#] end of __file__
