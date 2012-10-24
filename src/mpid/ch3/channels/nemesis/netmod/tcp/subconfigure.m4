[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[tcp],[build_nemesis_netmod_tcp=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_TCP],[test "X$build_nemesis_netmod_tcp" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
# nothing to do for tcp right now
dnl AM_COND_IF([BUILD_NEMESIS_NETMOD_TCP],[
dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:tcp])
dnl ])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_TCP,...)
])dnl end _BODY

[#] end of __file__
