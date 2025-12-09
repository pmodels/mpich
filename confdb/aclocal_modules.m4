dnl ==== mpl ====

dnl internal routine
AC_DEFUN([PAC_CONFIG_MPL_EMBEDDED],[
    mpl_subdir_args="--disable-versioning --enable-embedded"
    PAC_PUSH_FLAG([CFLAGS])
    if test -n "$VISIBILITY_CFLAGS" ; then
        CFLAGS="$CFLAGS $VISIBILITY_CFLAGS -DHAVE_VISIBILITY"
    fi
    PAC_CONFIG_SUBDIR_ARGS(mpl_embedded_dir,[$mpl_subdir_args],[],[AC_MSG_ERROR(MPL configure failed)])
    PAC_POP_FLAG([CFLAGS])
])

AC_DEFUN([PAC_CONFIG_MPL],[
    dnl NOTE: we only support embedded mpl
    mpl_srcdir=""
    mpl_includedir=""
    mpl_lib=""
    AC_SUBST([mpl_srcdir])
    AC_SUBST([mpl_includedir])
    AC_SUBST([mpl_lib])
    # Controls whether we recurse into the MPL dir when running "dist" rules like
    # "make distclean".  Technically we are cheating whenever DIST_SUBDIRS is not a
    # superset of SUBDIRS, but we don't want to double-distclean and similar.
    mpl_dist_srcdir=""
    AC_SUBST(mpl_dist_srcdir)

    PAC_CHECK_HEADER_LIB_EXPLICIT([mpl],[mpl.h],[mpl],[MPL_env2int])
    if test "$with_mpl" = "embedded" ; then
        m4_ifdef([MPICH_CONFIGURE], [
            dnl ---- the main MPICH configure ----
            PAC_CONFIG_MPL_EMBEDDED
            PAC_APPEND_FLAG([-I${main_top_builddir}/src/mpl/include], [CPPFLAGS])
            PAC_APPEND_FLAG([-I${use_top_srcdir}/src/mpl/include], [CPPFLAGS])
            PAC_APPEND_FLAG([${MPL_CFLAGS}], [CFLAGS])
            mpl_srcdir="src/mpl"
            mpl_lib="src/mpl/libmpl.la"
        ], [
            dnl ---- sub-configure (e.g. hydra, romio) ----
            if test "$FROM_MPICH" = "yes"; then
                dnl skip ROMIO since mpich already links libmpl.la
                if test "$pac_skip_mpl_lib" != "yes" ; then
                    mpl_lib="$main_top_builddir/src/mpl/libmpl.la"
                fi
                mpl_includedir="-I$main_top_builddir/src/mpl/include -I$main_top_srcdir/src/mpl/include"
                # source variables that are configured by MPL
                AC_MSG_NOTICE([sourcing $main_top_builddir/src/mpl/localdefs])
                . $main_top_builddir/src/mpl/localdefs
            elif test "$FROM_HYDRA" = "yes"; then
                m4_ifdef([HYDRA_CONFIGURE], [
                    PAC_CONFIG_MPL_EMBEDDED
                    mpl_srcdir="mpl_embedded_dir"
                    mpl_dist_srcdir="mpl_embedded_dir"
                    mpl_lib="mpl_embedded_dir/libmpl.la"
                    mpl_includedir='-I$(top_builddir)/mpl_embedded_dir/include -I$(top_srcdir)/mpl_embedded_dir/include'
                ], [
                    dnl both mpl and pmi are in modules/
                    mpl_includedir="-I$srcdir/../mpl/include -I../mpl/include"
                    AC_MSG_NOTICE([sourcing ../mpl/localdefs])
                    . ../mpl/localdefs
                ])
            else
                dnl stand-alone ROMIO
                PAC_CONFIG_MPL_EMBEDDED
                mpl_srcdir="mpl_embedded_dir"
                mpl_dist_srcdir="mpl_embedded_dir"
                mpl_lib="mpl_embedded_dir/libmpl.la"
                mpl_includedir='-I$(top_builddir)/mpl_embedded_dir/include -I$(top_srcdir)/mpl_embedded_dir/include'
            fi
        ])
    fi
])

dnl ==== hwloc ====

dnl internal routine, $1 is the extra cflags, hwloc_embedded_dir is m4 macro
dnl defined to be the path to embedded hwloc.
AC_DEFUN([PAC_CONFIG_HWLOC_EMBEDDED],[
    PAC_PUSH_FLAG([CFLAGS])
    CFLAGS="$USER_CFLAGS $1"
    hwloc_config_args="--enable-embedded-mode --disable-visibility"
    hwloc_config_args="$hwloc_config_args --disable-gl"
    hwloc_config_args="$hwloc_config_args --disable-libxml2"
    hwloc_config_args="$hwloc_config_args --disable-nvml"
    hwloc_config_args="$hwloc_config_args --disable-cuda"
    hwloc_config_args="$hwloc_config_args --disable-opencl"
    hwloc_config_args="$hwloc_config_args --disable-rsmi"
    # disable GPU support in embedded hwloc if explicitly disabled in MPICH
    # NOTE: there is no --disable-rocm option for the hwloc configure
    if test "$with_ze" = "no" ; then
        hwloc_config_args="$hwloc_config_args --disable-levelzero"
    fi
    if test "$with_cuda" = "no" ; then
        hwloc_config_args="$hwloc_config_args --disable-cuda"
    fi
    PAC_CONFIG_SUBDIR_ARGS(hwloc_embedded_dir, [$hwloc_config_args],[], [AC_MSG_ERROR(embedded hwloc configure failed)])
    PAC_POP_FLAG([CFLAGS])
])

AC_DEFUN([PAC_CONFIG_HWLOC],[
    dnl minor difference from e.g. mpl -- we'll prioritize system hwloc by default
    hwloc_srcdir=""
    hwloc_includedir=""
    hwloc_lib=""
    AC_SUBST([hwloc_srcdir])
    AC_SUBST([hwloc_includedir])
    AC_SUBST([hwloc_lib])

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
        # make sure subsystems such as hydra will use embedded hwloc consistently
        subsys_config_args="$subsys_config_args --with-hwloc=embedded"
    fi

    if test "$with_hwloc" = "embedded" ; then
        m4_ifdef([MPICH_CONFIGURE], [
            dnl ---- the main MPICH configure ----
            hwloc_lib="modules/hwloc/hwloc/libhwloc_embedded.la"
            if test -e "${use_top_srcdir}/modules/PREBUILT" -a -e "$hwloc_lib"; then
                hwloc_srcdir=""
            else
                hwloc_srcdir="${main_top_builddir}/modules/hwloc"
                PAC_CONFIG_HWLOC_EMBEDDED([$VISIBILITY_CFLAGS])
            fi
            PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/hwloc/include],[CPPFLAGS])
            PAC_APPEND_FLAG([-I${main_top_builddir}/modules/hwloc/include],[CPPFLAGS])
            hwloc_config_status="${main_top_builddir}/modules/hwloc/config.status"
        ], [
            dnl ---- sub-configure (hydra) ----
            if test "$FROM_MPICH" = "yes"; then
                hwloc_includedir="-I${main_top_srcdir}/modules/hwloc/include -I${main_top_builddir}/modules/hwloc/include"
                hwloc_lib="${main_top_builddir}/modules/hwloc/hwloc/libhwloc_embedded.la"
                hwloc_config_status="${main_top_builddir}/modules/hwloc/config.status"
            else
                PAC_CONFIG_HWLOC_EMBEDDED()
                dnl Note that single quote is intentional to pass the variable as is
                hwloc_srcdir="hwloc_embedded_dir"
                hwloc_includedir='-I${srcdir}/${hwloc_srcdir}/include -I${builddir}/${hwloc_srcdir}/include'
                hwloc_lib='${builddir}/${hwloc_srcdir}/hwloc/libhwloc_embedded.la'
                hwloc_config_status="${builddir}/${hwloc_srcdir}/config.status"
            fi
        ])

        # capture the line -- S["HWLOC_DARWIN_LDFLAGS"]=" -framework Foundation -framework IOKit"
        hwloc_darwin_ldflags=$(awk -F'"' '/^S."HWLOC_DARWIN_LDFLAGS"/ {print $[]4}' "$hwloc_config_status")
        if test -n "$hwloc_darwin_ldflags" ; then
            echo "hwloc_darwin_ldflags = $hwloc_darwin_ldflags"
            PAC_APPEND_FLAG([$hwloc_darwin_ldflags], [LDFLAGS])
            # we also need these flags for linking in the wrapper scripts
            PAC_APPEND_FLAG([$hwloc_darwin_ldflags], [WRAPPER_DEPENDENCY_LDFLAGS])
        fi
    fi
])

dnl ==== pmi ====

AC_DEFUN([PAC_CONFIG_PMI],[
    pmi_srcdir=""
    pmi_lib=""
    pmi_includedir=""
    AC_SUBST([pmi_srcdir])
    AC_SUBST([pmi_lib])
    AC_SUBST([pmi_includedir])

    m4_ifdef([MPICH_CONFIGURE], [
        pmi_srcdir="src/pmi"
        pmi_lib="src/pmi/libpmi.la"

        pmi_subdir_args=""
        if test "$with_pmilib" != "install" ; then
            pmi_subdir_args="--enable-embedded"
        fi
        if test "$pac_have_pmi1" = "yes" -o "$pac_have_pmi2" = "yes" ; then
            pmi_subdir_args="$pmi_subdir_args --disable-pmi1 --disable-pmi2"
        fi
        if test "$pac_have_pmix" = "yes" ; then
            pmi_subdir_args="$pmi_subdir_args --disable-pmix"
        fi
        PAC_CONFIG_SUBDIR_ARGS([src/pmi], [$pmi_subdir_args])

        PAC_APPEND_FLAG([-I${use_top_srcdir}/src/pmi/include], [CPPFLAGS])
        PAC_APPEND_FLAG([-I${main_top_builddir}/src/pmi/include], [CPPFLAGS])
        # let subconfigure know we has libpmiutil.la
        HAS_LIBPMIUTIL_LA=yes
        export HAS_LIBPMIUTIL_LA
    ] , [
        dnl hydra - need libpmiutil.la
        if test "$HAS_LIBPMIUTIL_LA" = "yes" ; then
            pmi_includedir="-I$main_top_srcdir/src/pmi/src"
            pmi_lib="$main_top_builddir/src/pmi/libpmiutil.la"
        else
            pmi_srcdir=pmi_embedded_dir
            pmi_includedir='-I$(top_srcdir)/pmi_embedded_dir/src'
            pmi_lib='$(top_builddir)/pmi_embedded_dir/libpmiutil.la'
            PAC_CONFIG_SUBDIR_ARGS(pmi_embedded_dir, [--enable-embedded])
        fi
    ])
])

dnl ==== json-c ====

AC_DEFUN([PAC_CONFIG_JSON],[
    json_srcdir=""
    json_lib=""
    AC_SUBST([json_srcdir])
    AC_SUBST([json_lib])

    PAC_CHECK_HEADER_LIB_EXPLICIT([json],[json.h],[json-c],[json_token_parse])
    if test "$with_json" = "embedded" ; then
        json_lib="modules/json-c/libjson-c.la"
        if test -e "${use_top_srcdir}/modules/PREBUILT" -a -e "$json_lib"; then
            json_srcdir=""
        else
            PAC_PUSH_ALL_FLAGS()
            PAC_RESET_ALL_FLAGS()
            PAC_CONFIG_SUBDIR_ARGS([modules/json-c],[--enable-embedded --disable-werror],[],[AC_MSG_ERROR(json-c configure failed)])
            PAC_POP_ALL_FLAGS()
            json_srcdir="${main_top_builddir}/modules/json-c"
        fi
        PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/json-c],[CPPFLAGS])
        PAC_APPEND_FLAG([-I${main_top_builddir}/modules/json-c],[CPPFLAGS])
    fi
])

dnl ==== yaksa ====

AC_DEFUN([PAC_CONFIG_YAKSA],[
    yaksa_srcdir=""
    yaksa_lib=""
    AC_SUBST([yaksa_srcdir])
    AC_SUBST([yaksa_lib])

    PAC_CHECK_HEADER_LIB_EXPLICIT([yaksa],[yaksa.h],[yaksa],[yaksa_init])
    if test "$with_yaksa" = "embedded" ; then
        yaksa_lib="yaksa_embedded_dir/libyaksa.la"
        if test ! -e "${use_top_srcdir}/modules/PREBUILT-yaksa"; then
            PAC_PUSH_ALL_FLAGS()
            PAC_RESET_ALL_FLAGS()
            # no need for libtool versioning when embedding YAKSA
            yaksa_subdir_args="--enable-embedded"
            PAC_CONFIG_SUBDIR_ARGS(yaksa_embedded_dir,[$yaksa_subdir_args],[],[AC_MSG_ERROR(YAKSA configure failed)])
            PAC_POP_ALL_FLAGS()
            yaksa_srcdir="yaksa_embedded_dir"
        fi
        PAC_APPEND_FLAG(-I${main_top_builddir}/yaksa_embedded_dir/src/frontend/include, [CPPFLAGS])
        PAC_APPEND_FLAG(-I${use_top_srcdir}/yaksa_embedded_dir/src/frontend/include, [CPPFLAGS])
    fi
])
