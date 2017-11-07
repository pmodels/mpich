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
    CH4 OFI netmod arguments:
            am-only          - Do not use UCX tagged or RMA communication.
            ],
            [ucx_netmod_args=$withval],
            [ucx_netmod_args=])

dnl Parse the device arguments
    SAVE_IFS=$IFS
    IFS=':'
    args_array=$ucx_netmod_args
    do_am_only=false
    echo "Parsing Arguments for OFI Netmod"
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
    dnl Use embedded libfabric if we specify to do so or we didn't specify and the source is present
    if test "${with_ucx}" = "embedded" ; then
        ucx_embedded="yes"
    elif test -z ${with_ucx} ; then
        if test -f ${use_top_srcdir}/src/mpid/ch4/netmod/ucx/ucx/configure ; then
            ucx_embedded="yes"
        else
            ucx_embedded="no"
            PAC_SET_HEADER_LIB_PATH(ucx)
        fi
    else
        ucx_embedded="no"
        PAC_SET_HEADER_LIB_PATH(ucx)
    fi

    if test "${ucx_embedded}" = "yes" ; then
        PAC_PUSH_FLAG(CPPFLAGS)
        PAC_CONFIG_SUBDIR_ARGS([src/mpid/ch4/netmod/ucx/ucx],[--disable-static --enable-embedded],[],[AC_MSG_ERROR(ucx configure failed)])
        PAC_POP_FLAG(CPPFLAGS)
        PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpid/ch4/netmod/ucx/ucx/src], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpid/ch4/netmod/ucx/ucx/src], [CPPFLAGS])

        ucxdir="src/mpid/ch4/netmod/ucx/ucx"
        ucxlib="src/mpid/ch4/netmod/ucx/ucx/src/ucp/libucp.la"
    else
        PAC_PUSH_FLAG(LIBS)
        PAC_CHECK_HEADER_LIB_FATAL(ucx, ucp/api/ucp.h, ucp, ucp_config_read)
        PAC_POP_FLAG(LIBS)
        PAC_APPEND_FLAG([-lucp -lucs],[WRAPPER_LIBS])
    fi

])dnl end AM_COND_IF(BUILD_CH4_NETMOD_OFI,...)
])dnl end _BODY

[#] end of __file__
