dnl **** MPL ************************************
dnl PAC_SUBDIR_MPL([embed_src_dir])
AC_DEFUN([PAC_SUBDIR_MPL],[
    PAC_SUBDIR_LIBNAME([MPLLIBNAME],[mpl])
    PAC_SET_HEADER_LIB_PATH_ARG([mpl])

    case $with_mpl in
        ../*)
            PAC_SUBDIR_EMBED_REUSE([$MPLLIBNAME],[$with_mpl],[$with_mpl],[$with_mpl/include])
            ;;
        *)
            PAC_SUBDIR_EMBED([$MPLLIBNAME],[$1],[$1],[$1/include])
            ;;
    esac
])

AC_DEFUN([PAC_CONFIG_ARG_MPL],[
    config_arg="--disable-versioning --enable-embedded"
])
