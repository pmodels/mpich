[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for net in $ch4_netmods ; do
            AS_CASE([$net],[portals4],[build_ch4_netmod_portals4=yes])
	    if test $net = "portals4" ; then
	       AC_DEFINE(HAVE_CH4_NETMOD_PORTALS4,1,[Portals4 netmod is built])
	       # if test "$build_ch4_locality_info" != "yes" ; then
	       #    AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
	       # 	  build_ch4_locality_info="yes"
	       # fi
	    fi
        done
    ])
    AM_CONDITIONAL([BUILD_CH4_NETMOD_PORTALS4],[test "X$build_ch4_netmod_portals4" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_PORTALS4],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:portals4])

    PAC_SET_HEADER_LIB_PATH(portals4)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB_FATAL(portals4, portals4.h, portals, PtlInit)
    PAC_APPEND_FLAG([-lportals],[WRAPPER_LIBS])
    PAC_POP_FLAG(LIBS)

])dnl end AM_COND_IF(BUILD_CH4_NETMOD_PORTALS4,...)
])dnl end _BODY

[#] end of __file__
