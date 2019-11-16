dnl Need it right after AC_INIT
AC_DEFUN([PAC_HWLOC_NEED_CANONICAL_TARGET],[
    # needed by hwloc in embedded mode.  Must come very early to avoid
    # bizarre expansion ordering warnings
    AC_CANONICAL_TARGET
])

dnl Between AC_PROG_CC and AM_INIT_AUTOMAKE
AC_DEFUN([PAC_HWLOC_NEED_USE_SYSTEM_EXTENSIONS],[
    # also needed by hwloc in embedded mode, must also come early for expansion
    # ordering reasons
    AC_USE_SYSTEM_EXTENSIONS

    # Define -D_DARWIN_C_SOURCE on OS/X to ensure that hwloc will build even if we
    # are building under MPICH with --enable-strict that defined _POSIX_C_SOURCE.
    # Some standard Darwin headers don't build correctly under a strict posix
    # environment.
    AS_CASE([$host],
        [*-*-darwin*], [PAC_APPEND_FLAG([-D_DARWIN_C_SOURCE],[CPPFLAGS])]
    )
])

dnl We used to call HWLOC_SETUP_CORE directly, which sets up 
dnl HWLOC_EMBEDDED_CFLAGS, HWLOC_EMBEDDED_CPPFLAGS, and HWLOC_EMBEDDED_LIBS.
dnl Now we uses PAC_CONFIG_SUBDIR_ARGS, we need hard code CPPFLAGS and 
dnl convenience_libs. The drawback is we need update them if hwloc changes 
dnl their convention upon new versions.
AC_DEFUN([PAC_SUBDIR_HWLOC],[
    PAC_ARG_WITH_PREFIX([hwloc])

    case $with_hwloc_prefix in
        ../*)
            use_embedded_hwloc=yes
            PAC_SUBDIR_EMBED_REUSE([hwloc_embedded],[$with_hwloc_prefix],[$with_hwloc_prefix/hwloc],[$with_hwloc_prefix/include])
            ;;
        embedded)
            use_embedded_hwloc=yes
            PAC_SUBDIR_EMBED([hwloc_embedded],[$1],[$1/hwloc],[$1/include])
            ;;
        *)
            if test "$with_hwloc_prefix" != "system" ; then
                PAC_SUBDIR_PREFIX([hwloc],[$with_hwloc_prefix],[wait])
            fi
            PAC_CHECK_SYSTEM_HWLOC
            if test "$have_hwloc" = "yes" ; then
                PAC_SUBDIR_SYSTEM([hwloc])
            fi
            ;;
    esac

    # FIXME: Disable hwloc on Cygwin for now. The hwloc package, atleast as of 1.0.2,
    # does not install correctly on Cygwin
    AS_CASE([$host], [*-*-cygwin], [have_hwloc=no])

    if test "$have_hwloc" = "yes" ; then
        AC_DEFINE(HAVE_HWLOC,1,[Define if hwloc is available])
    fi

    AM_CONDITIONAL([HAVE_HWLOC], [test "${have_hwloc}" = "yes"])

    m4_ifset([INSIDE_MPICH],[
        if test "$have_hwloc" = "yes" ; then
            PAC_EXTERNAL_NETLOC
        fi
    ])
])

AC_DEFUN([PAC_CONFIG_ARG_HWLOC], [
    config_arg="--enable-embedded-mode --enable-visibility"
])

AC_DEFUN([PAC_CHECK_SYSTEM_HWLOC],[
    AC_CHECK_HEADERS([hwloc.h])
    # hwloc_topology_set_pid was added in hwloc-1.0.0, which is our
    # minimum required version
    AC_CHECK_LIB([hwloc],[hwloc_topology_set_pid])
    AC_MSG_CHECKING([if non-embedded hwloc works])
    if test "$ac_cv_header_hwloc_h" = "yes" -a "$ac_cv_lib_hwloc_hwloc_topology_set_pid" = "yes" ; then
        have_hwloc=yes
    else
        have_hwloc=no
    fi
    AC_MSG_RESULT([$have_hwloc])

    # FIXME: Disable hwloc on Cygwin for now. The hwloc package,
    # atleast as of 1.0.2, does not install correctly on Cygwin
    AS_CASE([$host], [*-*-cygwin], [have_hwloc=no])
])

dnl **** NETLOC **************************
AC_DEFUN([PAC_EXTERNAL_NETLOC],[
    AC_ARG_WITH([netloc-prefix],
                [AS_HELP_STRING([[--with-netloc-prefix[=DIR]]],
                                [use the NETLOC library installed in DIR]) or system to use the system library], [],
                                [with_netloc_prefix=no])

    if test "${with_netloc_prefix}" != "no" ; then
        if test "${with_netloc_prefix}" = "system"; then
            PAC_CHECK_SYSTEM_NETLOC
        else
            PAC_SUBDIR_PREFIX_CHECK([Netloc],[$with_netloc_prefix],[include/netloc.h])
            PAC_SUBDIR_PREFIX([netloc],[$with_netloc_prefix])
        fi
        AC_DEFINE(HAVE_NETLOC,1,[Define if netloc is available in either user specified path or in system path])
    fi
])

AC_DEFUN([PAC_CHECK_SYSTEM_NETLOC],[
    AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "netloc.h"
        ],
        [])],
    [AC_MSG_ERROR([the Netloc installation seems to be added from system path])],
    [])
])

