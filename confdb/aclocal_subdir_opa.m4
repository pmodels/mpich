dnl PAC_SUBDIR_OPA([embed_src_dir])
AC_DEFUN([PAC_SUBDIR_OPA],[
    PAC_SUBDIR_LIBNAME([OPALIBNAME],[opa])
    PAC_SUBDIR_EMBED([$OPALIBNAME],[$1],[$1/src],[$1/src])
])

AC_DEFUN([PAC_CONFIG_ARG_OPA],[
    config_arg="--disable-versioning --enable-embedded"

    # OPA defaults to "auto", but in MPICH we want "auto_allow_emulation" to
    # easily permit using channels like ch3:sock that don't care about atomics
    AC_ARG_WITH([atomic-primitives],
                [AS_HELP_STRING([--with-atomic-primitives],
                                [Force OPA to use a specific atomic primitives
                                    implementation.  See the src/openpa directory
                                    for more info.])],
                [],[with_atomic_primitives=not_specified])
    if test "$with_atomic_primitives" = "not_specified" ; then
        config_arg="$config_arg --with-atomic-primitives=auto_allow_emulation"
    fi
])

