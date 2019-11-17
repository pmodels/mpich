dnl We used to call HWLOC_SETUP_CORE directly, which sets up 
dnl HWLOC_EMBEDDED_CFLAGS, HWLOC_EMBEDDED_CPPFLAGS, and HWLOC_EMBEDDED_LIBS.
dnl Now we uses PAC_CONFIG_SUBDIR_ARGS, we need hard code CPPFLAGS and 
dnl convenience_libs. The drawback is we need update them if hwloc changes 
dnl their convention upon new versions.
AC_DEFUN([PAC_SUBDIR_HWLOC],[
    PAC_SUBDIR_CONFIG([hwloc],[hwloc.h],[hwloc],[hwloc_topology_set_pid])
    PAC_SUBDIR_CONFIG_EMBED([hwloc],[hwloc_embedded],[$1],[hwloc],[include])

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

dnl **** NETLOC **************************
AC_DEFUN([PAC_EXTERNAL_NETLOC],[
    PAC_SET_HEADER_LIB_PATH_SIMPLE([netloc])
    AC_CHECK_HEADER([netloc.h], [have_netloc=yes], [have_netloc=no])
    if test $have_netloc = yes ; then
        AC_DEFINE(HAVE_NETLOC,1,[Define if netloc is available in either user specified path or in system path])
    fi
])
