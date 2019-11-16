AC_DEFUN([PAC_PROBE_UCX],[
    if test x"$with_ucx" != x"embedded" ; then
	PAC_CHECK_HEADER_LIB_ONLY([ucx],[ucp/api/ucp.h], [ucp],[ucp_config_read])
        if test -n "$with_ucx" && test $have_ucx = no ; then
            AC_MSG_ERROR([with-ucx specified but no ucx library was found])
        fi
    else
	have_ucx=no
    fi
])

dnl PAC_SUBDIR_UCX([embedded_path])
AC_DEFUN([PAC_SUBDIR_UCX],[
    dnl AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:ucx])

    ucx_embedded=""
    if test "$have_ucx" = "no" ; then
        ucx_embedded="yes"
    else
        ucx_embedded="no"
    fi

    if test "${ucx_embedded}" = "yes" ; then
        AC_MSG_NOTICE([CH4 OFI Netmod: Using an embedded ucx])
        PAC_SUBDIR_EMBED([ucp],[$1],[$1/src/ucp],[$1/src])
    else
        AC_MSG_NOTICE([CH4 OFI Netmod: Using an external ucx])
        PAC_SUBDIR_SYSTEM([ucp])
        PAC_SUBDIR_SYSTEM([ucs])
    fi
])

AC_DEFUN([PAC_CONFIG_ARG_UCX],[
    config_arg="--disable-static --enable-embedded"
])
