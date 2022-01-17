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
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4_NETMOD_UCX],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:ucx])

    AC_ARG_WITH([ch4-ucx-rankbits],
                AS_HELP_STRING([--with-ch4-ucx-rankbits=<N>],[Number of bits allocated to the rank field]),
                [ rankbits=$withval ],
                [ rankbits=16 ])
    if test "$rankbits" -lt "16" -a "$rankbits" -gt "32" ; then
        AC_MSG_ERROR(ch4-ucx-rankbits must be between 16 and 32-bit)
    fi
    AC_DEFINE_UNQUOTED(CH4_UCX_RANKBITS,$rankbits,[Define the number of rank bits used in UCX])

    ucxdir=""
    AC_SUBST([ucxdir])
    ucxlib=""
    AC_SUBST([ucxlib])

    if test "$pac_have_ucx" = "no" ; then
        with_ucx=embedded
    fi
    if test "$with_ucx" = "embedded" ; then
        ucxlib="modules/ucx/src/ucp/libucp.la"
        if test -e "${use_top_srcdir}/modules/PREBUILT" -a -e "$ucxlib"; then
            ucxdir=""
        else
            PAC_PUSH_ALL_FLAGS()
            PAC_RESET_ALL_FLAGS()
            if test "$enable_fast" = "yes" -o "$enable_fast" = "all" ; then
                # add flags from contrib/configure-release and contrib/configure-opt scripts in the ucx source
                ucx_opt_flags="--disable-logging --disable-debug --disable-assertions --disable-params-check --enable-optimizations"
            else
                ucx_opt_flags=""
            fi
            PAC_CONFIG_SUBDIR_ARGS([modules/ucx],[--disable-static --enable-embedded --with-java=no $ucx_opt_flags],[],[AC_MSG_ERROR(ucx configure failed)])
            PAC_POP_ALL_FLAGS()
            ucxdir="modules/ucx"
        fi
        PAC_APPEND_FLAG([-I${main_top_builddir}/modules/ucx/src], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/ucx/src], [CPPFLAGS])

    else
        dnl PAC_PROBE_HEADER_LIB must've been successful
        AC_MSG_NOTICE([CH4 UCX Netmod:  Using an external ucx])

        dnl require UCX >= 1.9.0 for tagged send/recv nbx APIs
        AC_MSG_CHECKING([if UCX meets minimum version requirement])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <ucp/api/ucp.h>], [
                           #if UCP_VERSION(UCP_API_MAJOR, UCP_API_MINOR) < UCP_VERSION(1, 9)
                           #error
                           #endif
                           return 0;])],[ucx_happy=yes],[ucx_happy=no])
        AC_MSG_RESULT([$ucx_happy])

        # if a too old UCX was found, throw an error
        if test "$ucx_happy" = "no" ; then
            AC_MSG_ERROR([UCX installation does not meet minimum version requirement (v1.9.0). Please upgrade your installation, or use --with-ucx=embedded.])
        fi
        PAC_LIBS_ADD([-lucp -lucs])
    fi
])dnl end AM_COND_IF(BUILD_CH4_NETMOD_UCX,...)
])dnl end _BODY

[#] end of __file__
