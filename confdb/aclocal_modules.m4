dnl ==== hwloc ====

dnl internal routine, $1 is the extra cflags, hwloc_embedded_dir is m4 macro
dnl defined to be the path to embedded hwloc.
AC_DEFUN([PAC_CONFIG_HWLOC_EMBEDDED],[
    PAC_PUSH_FLAG([CFLAGS])
    CFLAGS="$1"
    hwloc_config_args="--enable-embedded-mode --disable-visibility"
    hwloc_config_args="$hwloc_config_args --disable-libxml2"
    hwloc_config_args="$hwloc_config_args --disable-nvml"
    hwloc_config_args="$hwloc_config_args --disable-cuda"
    hwloc_config_args="$hwloc_config_args --disable-opencl"
    hwloc_config_args="$hwloc_config_args --disable-rsmi"
    PAC_CONFIG_SUBDIR_ARGS(hwloc_embedded_dir, [$hwloc_config_args],[], [AC_MSG_ERROR(embedded hwloc configure failed)])
    PAC_POP_FLAG([CFLAGS])
])

AC_DEFUN([PAC_CONFIG_HWLOC],[
    dnl minor difference from e.g. mpl and zm -- we'll prioritize system hwloc by default
    PAC_CHECK_HEADER_LIB_OPTIONAL([hwloc],[hwloc.h],[hwloc],[hwloc_topology_set_pid])
    if test "$pac_have_hwloc" = "yes" -a "$with_hwloc" != "embedded"; then
        AC_MSG_CHECKING([if hwloc meets minimum version requirement])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <hwloc.h>], [
                        #if HWLOC_API_VERSION < 0x00020000
                        #error
                        #endif
                        return 0;])],[],[pac_have_hwloc=no])
        AC_MSG_RESULT([$pac_have_hwloc])

        # if an old hwloc was specified by the user, throw an error
        if test "$pac_have_hwloc" = "no" -a -n "$with_hwloc" -a "$with_hwloc" != "yes" ; then
            AC_MSG_ERROR([hwloc installation does not meet minimum version requirement (2.0). Please update your hwloc installation or use --with-hwloc=embedded.])
        fi
    fi
    if test "$pac_have_hwloc" = "no" -a "$with_hwloc" != "no"; then
        with_hwloc=embedded
        pac_have_hwloc=yes
    fi

    if test "$with_hwloc" = "embedded" ; then
        m4_if(hwloc_embedded_dir, [modules/hwloc], [
            dnl ---- the main MPICH configure ----
            PAC_CONFIG_HWLOC_EMBEDDED([$VISIBILITY_CFLAGS])
            hwlocsrcdir="${main_top_builddir}/modules/hwloc"
            hwloclib="${main_top_builddir}/modules/hwloc/hwloc/libhwloc_embedded.la"
            PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/hwloc/include],[CPPFLAGS])
            PAC_APPEND_FLAG([-I${main_top_builddir}/modules/hwloc/include],[CPPFLAGS])

            # capture the line -- S["HWLOC_EMBEDDED_LIBS"]="-lm "
            hwloc_embedded_libs=$(awk -F'"' '/^S."HWLOC_EMBEDDED_LIBS"/ {print $4}' modules/hwloc/config.status)
            echo "hwloc_embedded_libs = $hwloc_embedded_libs"
            if test -n "$hwloc_embedded_libs" ; then
                dnl TODO: split and add individual lib
                PAC_LIBS_ADD([$hwloc_embedded_libs])
            fi
        ], [
            dnl ---- sub-configure (hydra) ----
            if test "$FROM_MPICH" = "yes"; then
                hwloc_includedir="-I${main_top_srcdir}/modules/hwloc/include -I${main_top_builddir}/modules/hwloc/include"
                hwloc_lib="${main_top_builddir}/modules/hwloc/hwloc/libhwloc_embedded.la"
            else
                PAC_CONFIG_HWLOC_EMBEDDED()
                dnl Note that single quote is intentional to pass the variable as is
                hwloc_srcdir="hwloc_embedded_dir"
                hwloc_includedir='-I${srcdir}/${hwloc_srcdir}/include -I${builddir}/${hwloc_srcdir}/include'
                hwloc_lib='${builddir}/${hwloc_srcdir}/hwloc/libhwloc_embedded.la'
            fi
        ])
    fi
])
