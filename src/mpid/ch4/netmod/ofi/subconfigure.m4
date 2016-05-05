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

        AC_ARG_WITH(ch4-ofi-direct-provider,
            [  --with-ch4-ofi-direct-provider=provider
                provider - Build OFI with FABRIC_DIRECT mode using the specified provider
                           Provider value does not matter if not building an embedded OFI library
            ],
            [ofi_direct_provider=$withval],
            [ofi_direct_provider=])

    if [test "x$ofi_direct_provider" != "x"]; then
       AC_MSG_NOTICE([Enabling OFI netmod direct provider])
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

    runtime_capabilities="no"
    no_providers="no"
    # $netmod_args - contains the OFI provider
    if test "${netmod_args#* }" != "$netmod_args" ; then
        runtime_capabilities="yes"
        AC_MSG_NOTICE([Using runtime capability set due to multiple selected providers])
    elif test "x$netmod_args" = "x" || test "$netmod_args" = "runtime"; then
        runtime_capabilities="yes"
        no_providers="yes"
        AC_MSG_NOTICE([Using runtime capability set due to no selected provider or explicity runtime selection])
    fi

    if test "$no_providers" = "no" ; then
        enable_psm="no"
        enable_psm2="no"
        enable_sockets="no"
        enable_verbs="no"
        enable_usnic="no"
        enable_mxm="no"
        enable_gni="no"
        enable_bgq="no"
        enable_udp="no"
        enable_rxm="no"
        enable_rxd="no"
    else
        enable_psm="yes"
        enable_psm2="yes"
        enable_sockets="yes"
        enable_verbs="yes"
        enable_usnic="yes"
        enable_mxm="yes"
        enable_gni="yes"
        enable_bgq="yes"
        enable_udp="yes"
        enable_rxm="yes"
        enable_rxd="yes"
    fi

    for provider in $netmod_args ; do
        dnl For these providers, we know which capabilities we want to select by default.
        dnl We want to enable compiling with them, but we don't need to enable runtime checks.
        if test "$provider" = "psm" ; then
            enable_psm="yes"
        fi
        if test "$provider" = "psm2" || test "$provider" = "opa" ; then
            enable_psm2="yes"
        fi
        if test "$provider" = "sockets" ; then
            enable_sockets="yes"
        fi
        if test "$provider" = "gni" ; then
            enable_gni="yes"
        fi
        if test "$provider" = "bgq" ; then
            enable_bgq="yes"
        fi

        dnl For these providers, we don't know exactly which capabilities we
        dnl want to select by default so we turn on runtime checks. At some point
        dnl in the future, we may create a specfici capability set for them.
        if test "$provider" = "verbs" ; then
            enable_verbs="yes"
            runtime_capabilities="yes"
        fi
        if test "$provider" = "usnic" ; then
            enable_usnic="yes"
            runtime_capabilities="yes"
        fi
        if test "$provider" = "mxm" ; then
            enable_mxm="yes"
            runtime_capabilities="yes"
        fi
        if test "$provider" = "udp" ; then
            enable_udp="yes"
            runtime_capabilities="yes"
        fi
        if test "$provider" = "rxm" ; then
            enable_rxm="yes"
            runtime_capabilities="yes"
        fi
        if test "$provider" = "rxd" ; then
            enable_rxd="yes"
            runtime_capabilities="yes"
        fi
    done

    if test "$runtime_capabilities" = "yes" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "psm" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_PSM], [1], [Define to use PSM capability set])
        enable_psm="yes"
    elif test "$netmod_args" = "psm2" || test "$netmod_args" = "opa" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_PSM2], [1], [Define to use PSM2 capability set])
        enable_psm2="yes"
    elif test "$netmod_args" = "sockets" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_SOCKETS], [1], [Define to use sockets capability set])
        enable_sockets="yes"
    elif test "$netmod_args" = "verbs" ; then
        enable_verbs="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "usnic" ; then
        enable_usnic="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "mxm" ; then
        enable_mxm="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "gni" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_GNI], [1], [Define to use gni capability set])
        enable_gni="yes"
    elif test "$netmod_args" = "bgq" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_BGQ], [1], [Define to use bgq capability set])
        enable_bgq="yes"
    elif test "$netmod_args" = "udp" ; then
        enable_udp="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "rxm" ; then
        enable_rxm="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    elif test "$netmod_args" = "rxd" ; then
        enable_rxd="yes"
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    fi

    if test "${ofi_embedded}" = "yes" ; then
        AC_MSG_NOTICE([CH4 OFI Netmod:  Using an embedded libfabric])
        ofi_subdir_args="--enable-embedded"

        prov_config=""
        if test "x${netmod_args}" != "x" ; then
            prov_config+=" --enable-psm=${enable_psm}"
            prov_config+=" --enable-psm2=${enable_psm2}"
            prov_config+=" --enable-sockets=${enable_sockets}"
            prov_config+=" --enable-verbs=${enable_verbs}"
            prov_config+=" --enable-usnic=${enable_usnic}"
            prov_config+=" --enable-mxm=${enable_mxm}"
            prov_config+=" --enable-gni=${enable_gni}"
            prov_config+=" --enable-bgq=${enable_bgq}"
            prov_config+=" --enable-udp=${enable_udp}"
            prov_config+=" --enable-rxm=${enable_rxm}"
            prov_config+=" --enable-rxd=${enable_rxd}"
        fi

        if test "x${ofi_direct_provider}" != "x" ; then
            prov_config+=" --enable-direct=${ofi_direct_provider}"
            AC_MSG_NOTICE([Enabling direct embedded provider: ${ofi_direct_provider}])
        fi

        ofi_subdir_args+=" $prov_config"

        dnl Unset all of these env vars so they don't pollute the libfabric configuration
        PAC_PUSH_FLAG(CPPFLAGS)
        PAC_CONFIG_SUBDIR_ARGS([src/mpid/ch4/netmod/ofi/libfabric],[$ofi_subdir_args],[],[AC_MSG_ERROR(libfabric configure failed)])
        PAC_POP_FLAG(CPPFLAGS)
        PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpid/ch4/netmod/ofi/libfabric/include], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/include], [CPPFLAGS])

        if test "x$ofi_direct_provider" != "x" ; then
            PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpid/ch4/netmod/ofi/libfabric/prov/${ofi_direct_provider}/include], [CPPFLAGS])
            PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/prov/${ofi_direct_provider}/include], [CPPFLAGS])
            PAC_APPEND_FLAG([-DFABRIC_DIRECT],[CPPFLAGS])
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
    PAC_APPEND_FLAG([-lm],[LDFLAGS])

])dnl end AM_COND_IF(BUILD_CH4_NETMOD_OFI,...)
])dnl end _BODY

[#] end of __file__
