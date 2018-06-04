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
    elif test -z "${with_libfabric}" && test -z "${with_libfabric_lib}" && test -z "${with_libfabric_include}" ; then
        if test -f ${use_top_srcdir}/src/mpid/ch4/netmod/ofi/libfabric/configure ; then
            ofi_embedded="yes"
        else
            ofi_embedded="no"
            PAC_SET_HEADER_LIB_PATH(libfabric)
        fi
    else
        ofi_embedded="no"
        PAC_SET_HEADER_LIB_PATH(libfabric)
        AC_SUBST([with_libfabric])
    fi
    AC_SUBST([ofi_embedded])

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
        enable_gni="no"
        enable_bgq="no"
        enable_udp="no"
        enable_rxm="no"
        enable_rxd="no"
        enable_tcp="no"
        enable_shm="no"
        enable_mlx="no"
    else
        enable_psm="yes"
        enable_psm2="yes"
        enable_sockets="yes"
        enable_verbs="yes"
        enable_usnic="yes"
        enable_gni="yes"
        enable_bgq="yes"
        enable_udp="yes"
        enable_rxm="yes"
        enable_rxd="yes"
        enable_tcp="yes"
        enable_shm="yes"
        enable_mlx="yes"
    fi

    for provider in $netmod_args ; do
        case "$provider" in
            dnl For these providers, we know which capabilities we want to select by default.
            dnl We want to enable compiling with them, but we don't need to enable runtime checks.
            "psm")
                enable_psm="yes"
                ;;
            "psm2" | "opa")
                enable_psm2="yes"
                ;;
            "sockets")
                enable_sockets="yes"
                ;;
            "gni")
                enable_gni="yes"
                ;;
            "bgq")
                enable_bgq="yes"
                ;;

            dnl For these providers, we don't know exactly which capabilities we
            dnl want to select by default so we turn on runtime checks. At some point
            dnl in the future, we may create a specific capability set for them.
            "verbs")
                enable_verbs="yes"
                runtime_capabilities="yes"
                ;;
            "usnic")
                enable_usnic="yes"
                runtime_capabilities="yes"
                ;;
            "udp")
                enable_udp="yes"
                runtime_capabilities="yes"
                ;;
            "rxm")
                enable_rxm="yes"
                runtime_capabilities="yes"
                ;;
            "rxd")
                enable_rxd="yes"
                runtime_capabilities="yes"
                ;;
            "tcp")
                enable_tcp="yes"
                runtime_capabilities="yes"
                ;;
            "shm")
                enable_shm="yes"
                runtime_capabilities="yes"
                ;;
            "mlx")
                enable_mlx="yes"
                runtime_capabilities="yes"
                ;;
            *)
                AC_MSG_WARN("Invalid provider $provider")
        esac
    done

    if test "$runtime_capabilities" = "yes" ; then
        AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
    else
        case "$netmod_args" in
            "psm")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_PSM], [1], [Define to use PSM capability set])
                enable_psm="yes"
                ;;
            "psm2" | "opa")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_PSM2], [1], [Define to use PSM2 capability set])
                enable_psm2="yes"
                ;;
            "sockets")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_SOCKETS], [1], [Define to use sockets capability set])
                enable_sockets="yes"
                ;;
            "gni")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_GNI], [1], [Define to use gni capability set])
                enable_gni="yes"
                ;;
            "bgq")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_BGQ], [1], [Define to use bgq capability set])
                enable_bgq="yes"
                ;;
            "verbs")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_verbs="yes"
                ;;
            "usnic")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_usnic="yes"
                ;;
            "udp")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_udp="yes"
                ;;
            "rxm")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_rxm="yes"
                ;;
            "rxd")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_rxd="yes"
                ;;
            "tcp")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_tcp="yes"
                ;;
            "shm")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_shm="yes"
                ;;
            "mlx")
                AC_DEFINE([MPIDI_CH4_OFI_USE_SET_RUNTIME], [1], [Define to use runtime capability set])
                enable_mlx="yes"
                ;;
            *)
                AC_MSG_WARN("Invalid provider $netmod_args")
        esac
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
            prov_config+=" --enable-gni=${enable_gni}"
            prov_config+=" --enable-bgq=${enable_bgq}"
            prov_config+=" --enable-udp=${enable_udp}"
            prov_config+=" --enable-rxm=${enable_rxm}"
            prov_config+=" --enable-rxd=${enable_rxd}"
            prov_config+=" --enable-tcp=${enable_tcp}"
            prov_config+=" --enable-shm=${enable_shm}"
            prov_config+=" --enable-mlx=${enable_mlx}"
        fi

        if test "x${ofi_direct_provider}" != "x" ; then
            prov_config+=" --enable-direct=${ofi_direct_provider}"
            AC_MSG_NOTICE([Enabling direct embedded provider: ${ofi_direct_provider}])
        fi

        ofi_subdir_args+=" $prov_config"

        dnl Unset all of these env vars so they don't pollute the libfabric configuration
        PAC_PUSH_ALL_FLAGS()
        PAC_RESET_ALL_FLAGS()
        PAC_CONFIG_SUBDIR_ARGS([src/mpid/ch4/netmod/ofi/libfabric],[$ofi_subdir_args],[],[AC_MSG_ERROR(libfabric configure failed)])
        PAC_POP_ALL_FLAGS()
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

    # Check for required functions

    # Does MPL provide MPL_aligned_malloc?
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <mplconfig.h>]],
                                       [[
                                       #ifndef MPL_DEFINE_ALIGNED_ALLOC
                                       # error
                                       #endif
                                       ]])],
                                       [],
                                       [AC_MSG_ERROR(MPL_aligned_alloc is required to build OFI netmod)])

])dnl end AM_COND_IF(BUILD_CH4_NETMOD_OFI,...)
])dnl end _BODY

[#] end of __file__
