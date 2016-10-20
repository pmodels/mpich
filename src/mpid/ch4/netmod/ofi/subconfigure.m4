[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for net in $ch4_netmods ; do
            AS_CASE([$net],[ofi],[build_ch4_netmod_ofi=yes])
	    if test $net = "ofi" ; then
	       AC_DEFINE(HAVE_CH4_NETMOD_OFI,1,[OFI netmod is built])
           AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
	    fi
        done

        AC_ARG_WITH(ch4-netmod-ofi-args,
        [  --with-ch4-netmod-ofi-args=arg1:arg2:arg3
        CH4 OFI netmod arguments:
                scalable-endpoints - Use OFI scalable endpoint mode
                av-table           - Use OFI AV table (logical addressing mode).  Default is av-map mode
                mr-basic           - USE OFI MR_BASIC mode. Default is MR_SCALABLE mode.
                direct-provider    - USE OFI FI_DIRECT to compile in a single OFI direct provider
                no-tagged          - Do not use OFI fi_tagged interfaces.
                no-data            - Disable immediate data field
                no-stx-rma         - Disable per-window EP & counter for RMA
                ],
                [ofi_netmod_args=$withval],
                [ofi_netmod_args=])

dnl Parse the device arguments
    SAVE_IFS=$IFS
    IFS=':'
    args_array=$ofi_netmod_args
    do_scalable_endpoints=false
    do_direct_provider=false
    do_av_table=false
    do_tagged=true
    do_data=true
    do_stx_rma=true
    do_mr_scalable=true
    echo "Parsing Arguments for OFI Netmod"
    for arg in $args_array; do
    case ${arg} in
      scalable-endpoints)
              do_scalable_endpoints=true
              echo " ---> CH4::OFI Provider : $arg"
              ;;
      av_table)
              do_av_table=true
              echo " ---> CH4::OFI Provider AV table : $arg"
              ;;
      direct-provider)
              do_direct_provider=true
              echo " ---> CH4::OFI Direct OFI Provider requested : $arg"
              ;;
      no-tagged)
              do_tagged=false
              echo " ---> CH4::OFI Disable fi_tagged interfaces : $arg"
              ;;
      no-data)
              do_data=false
              echo " ---> CH4::OFI Disable immediate data field : $arg"
	      ;;
      no-stx-rma)
              do_stx_rma=false
              echo " ---> CH4::OFI Disable per-window EP & counter for RMA : $arg"
	      ;;
      mr-basic)
              do_mr_scalable=false
              echo " ---> CH4::OFI Switching to MR_BASIC mode : $arg"
	      ;;
    esac
    done
    IFS=$SAVE_IFS

    if [test "$do_scalable_endpoints" = "true"]; then
       AC_MSG_NOTICE([Enabling OFI netmod scalable endpoints])
       PAC_APPEND_FLAG([-DMPIDI_OFI_CONFIG_USE_SCALABLE_ENDPOINTS], [CPPFLAGS])
    fi

    if [test "$do_av_table" = "true"]; then
       AC_MSG_NOTICE([Enabling OFI netmod AV table])
       PAC_APPEND_FLAG([-DMPIDI_OFI_CONFIG_USE_AV_TABLE], [CPPFLAGS])
    fi

    if [test "$do_direct_provider" = "true"]; then
       AC_MSG_NOTICE([Enabling OFI netmod direct provider])
       PAC_APPEND_FLAG([-DFABRIC_DIRECT],[CPPFLAGS])
    fi

    if [test "$do_tagged" = "true"]; then
       AC_DEFINE([USE_OFI_TAGGED], [1], [Define to use fi_tagged interfaces])
       AC_MSG_NOTICE([Enabling fi_tagged interfaces])
    fi

    if [test "$do_data" = "true"]; then
       AC_DEFINE([USE_OFI_IMMEDIATE_DATA], [1], [Define to use immediate data field])
       AC_MSG_NOTICE([Enabling immediate data field])
    fi

    if [test "$do_stx_rma" = "true"]; then
       AC_DEFINE([USE_OFI_STX_RMA], [1], [Define to use per-window EP & counter])
       AC_MSG_NOTICE([Enabling per-window EP & counter])
    fi

    if [test "$do_mr_scalable" = "true"]; then
       AC_DEFINE([USE_OFI_MR_SCALABLE], [1], [Define to use MR_SCALABLE])
       AC_MSG_NOTICE([Enabling MR_SCALABLE])
    fi
])
    AM_CONDITIONAL([BUILD_CH4_NETMOD_OFI],[test "X$build_ch4_netmod_ofi" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_OFI],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:ofi])

    ofisrcdir=""
    AC_SUBST([ofisrcdir])
    ofilib=""
    AC_SUBST([ofilib])

    ofi_embedded=""
    dnl Use embedded libfabric if we specify to do so or we didn't specify and the source is present
    if test "${with_libfabric}" = "embedded" ; then
        ofi_embedded="yes"
    elif test -z ${with_libfabric} ; then
        if test -f ${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/configure ; then
            ofi_embedded="yes"
        else
            ofi_embedded="no"
            PAC_SET_HEADER_LIB_PATH(libfabric)
        fi
    else
        ofi_embedded="no"
        PAC_SET_HEADER_LIB_PATH(libfabric)
    fi

    if test "${ofi_embedded}" = "yes" ; then
        AC_MSG_NOTICE([CH4 OFI Netmod:  Using an embedded libfabric])
        ofi_subdir_args="--enable-embedded"

        dnl Unset all of these env vars so they don't pollute the libfabric configuration
        PAC_PUSH_ALL_FLAGS()
        PAC_RESET_ALL_FLAGS()
        PAC_APPEND_FLAG([$VISIBILITY_CFLAGS], [CFLAGS])
        PAC_CONFIG_SUBDIR_ARGS([src/mpid/ch4/netmod/ofi/libfabric],[$ofi_subdir_args],[],[AC_MSG_ERROR(libfabric configure failed)])
        PAC_POP_ALL_FLAGS()
        PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpid/ch4/netmod/ofi/libfabric/include], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/include], [CPPFLAGS])

        if test "$do_direct_provider" = "true" ; then
            if test "x${enable_direct}" != "x" && test "${enable_direct}" != "no" ; then
                PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpid/ch4/netmod/ofi/libfabric/prov/${enable_direct}/include], [CPPFLAGS])
                PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/prov/${enable_direct}/include], [CPPFLAGS])
            else
                AC_MSG_ERROR([Enabled direct OFI provider but didn't specify which one])
            fi
        fi

        ofisrcdir="${master_top_builddir}/src/mpid/ch4/netmod/ofi/libfabric"
        ofilib="src/mpid/ch4/netmod/ofi/libfabric/src/libfabric.la"
    else
        PAC_PUSH_FLAG(LIBS)
        PAC_CHECK_HEADER_LIB([rdma/fabric.h], [fabric], [fi_getinfo], [have_libfabric=yes], [have_libfabric=no])
        PAC_POP_FLAG(LIBS)
        if test "${have_libfabric}" = "yes" ; then
            AC_MSG_NOTICE([CH4 OFI Netmod:  Using an external libfabric])
            PAC_APPEND_FLAG([-lfabric],[WRAPPER_LIBS])
        else
            AC_MSG_ERROR([Provided libfabric installation (--with-libfabric=${with_libfabric}) could not be configured.])
        fi
    fi

])dnl end AM_COND_IF(BUILD_CH4_NETMOD_OFI,...)
])dnl end _BODY

[#] end of __file__
