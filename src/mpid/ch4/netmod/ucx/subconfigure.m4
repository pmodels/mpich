[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for net in $ch4_netmods ; do
            AS_CASE([$net],[ucx],[build_ch4_netmod_ucx=yes])
	    if test $net = "ucx" ; then
	       AC_DEFINE(HAVE_CH4_NETMOD_UCX,1,[UCX netmod is built])
	       if test "$build_ch4_locality_info" != "yes" ; then
	          AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
		  build_ch4_locality_info="yes"
	       fi
	    fi
        done
    ])
    AM_CONDITIONAL([BUILD_CH4_NETMOD_UCX],[test "X$build_ch4_netmod_ucx" = "Xyes"])

    AC_ARG_WITH(ch4-netmod-ucx-args,
    [  --with-ch4-netmod-ucx-args=arg1:arg2:arg3
    CH4 UCX netmod arguments:
            am-only          - Do not use UCX tagged or RMA communication.
            ],
            [ucx_netmod_args=$withval],
            [ucx_netmod_args=])

dnl Parse the device arguments
    SAVE_IFS=$IFS
    IFS=':'
    args_array=$ucx_netmod_args
    do_am_only=false
    echo "Parsing Arguments for UCX Netmod"
    for arg in $args_array; do
    case ${arg} in
      am-only)
              do_am_only=true
              echo " ---> CH4::UCX Disable native tagged and RMA communication : $arg"
    esac
    done
    IFS=$SAVE_IFS

    if [test "$do_am_only" = "true"]; then
       AC_MSG_NOTICE([Disabling native UCX tagged and RMA communication])
       PAC_APPEND_FLAG([-DMPICH_UCX_AM_ONLY], [CPPFLAGS])
    fi
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_UCX],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:ucx])

    ucxdir=""
    AC_SUBST([ucxdir])
    ucxlib=""
    AC_SUBST([ucxlib])

    ucx_embedded=""
    if test $have_ucx = no ; then
        ucx_embedded="yes"
    fi
    
    if test "${ucx_embedded}" = "yes" ; then
        PAC_PUSH_FLAG(CPPFLAGS)
        PAC_CONFIG_SUBDIR_ARGS([modules/ucx],[--disable-static --enable-embedded],[],[AC_MSG_ERROR(ucx configure failed)])
        PAC_POP_FLAG(CPPFLAGS)
        PAC_APPEND_FLAG([-I${master_top_builddir}/modules/ucx/src], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/ucx/src], [CPPFLAGS])

        ucxdir="modules/ucx"
        ucxlib="modules/ucx/src/ucp/libucp.la"

        # embedded ucx is 1.4 or higher version, thus always set as defined.
        have_ucp_put_nb=yes
        have_ucp_get_nb=yes
    else
        PAC_APPEND_FLAG([-lucp -lucs],[WRAPPER_LIBS])

        # ucp_put_nb and ucp_get_nb are added only from ucx 1.4.
        PAC_CHECK_HEADER_LIB([ucp/api/ucp.h],[ucp],[ucp_put_nb], [have_ucp_put_nb=yes], [have_ucp_put_nb=no])
        PAC_CHECK_HEADER_LIB([ucp/api/ucp.h],[ucp],[ucp_get_nb], [have_ucp_get_nb=yes], [have_ucp_get_nb=no])
    fi

    if test "${have_ucp_put_nb}" = "yes" ; then
        AC_DEFINE(HAVE_UCP_PUT_NB,1,[Define if ucp_put_nb is defined in ucx])
    fi
    if test "${have_ucp_get_nb}" = "yes" ; then
        AC_DEFINE(HAVE_UCP_GET_NB,1,[Define if ucp_get_nb is defined in ucx])
    fi
])dnl end AM_COND_IF(BUILD_CH4_NETMOD_UCX,...)
])dnl end _BODY

[#] end of __file__
