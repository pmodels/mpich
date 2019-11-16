dnl **** MPL ************************************
dnl PAC_SUBDIR_MPL([embed_src_dir])
AC_DEFUN([PAC_SUBDIR_MPL],[
    PAC_SUBDIR_LIBNAME([MPLLIBNAME],[mpl])
    PAC_ARG_WITH_PREFIX([mpl])

    case $with_mpl_prefix in
        ../*)
            PAC_SUBDIR_EMBED_REUSE([$MPLLIBNAME],[$with_mpl_prefix],[$with_mpl_prefix],[$with_mpl_prefix/include])
            ;;
        embedded)
            PAC_SUBDIR_EMBED([$MPLLIBNAME],[$1],[$1],[$1/include])
            ;;
        *)
            PAC_SUBDIR_PREFIX_CHECK([MPL],[$with_mpl_prefix],[include/mplconfig.h])
            PAC_SUBDIR_PREFIX([$MPLLIBNAME],[$with_mpl_prefix])
            ;;
    esac
])

AC_DEFUN([PAC_CONFIG_ARG_MPL],[
    config_arg="--disable-versioning --enable-embedded"
])
