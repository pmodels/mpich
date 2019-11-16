dnl PAC_SUBDIR_OPA([embed_src_dir])
AC_DEFUN([PAC_SUBDIR_OPA],[
    PAC_SUBDIR_LIBNAME([OPALIBNAME],[opa])
    PAC_ARG_WITH_PREFIX([openpa])

    if test "$with_openpa_prefix" = "default" ; then
        PAC_SUBDIR_TEST_SYSTEM_OPA
    fi

    if test "$with_openpa_prefix" = "embedded" ; then
        PAC_SUBDIR_EMBED([$OPALIBNAME],[$1],[$1/src],[$1/src])
    elif test "$with_openpa_prefix" = "system" ; then
        PAC_SUBDIR_SYSTEM([$OPALIBNAME])
    else
        PAC_SUBDIR_PREFIX_CHECK([OpenPA],[$with_openpa_prefix],[include/opa_config.h])
        PAC_SUBDIR_PREFIX_CHECK([OpenPA],[$with_openpa_prefix],[include/opa_primitives.h])
        PAC_SUBDIR_PREFIX([$OPALIBNAME],[$with_openpa_prefix])
    fi
])

AC_DEFUN([PAC_SUBDIR_TEST_SYSTEM_OPA],[
    # see if OPA is already installed on the system
    PAC_PUSH_FLAG([LIBS])
    PAC_PREPEND_FLAG([-l${OPALIBNAME}],[LIBS])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
        #include "opa_primitives.h"
        ],[
        OPA_int_t i;
        OPA_store_int(i,10);
        OPA_fetch_and_incr_int(&i,5);
        ])dnl
        ],
        [with_openpa_prefix=system],
        [with_openpa_prefix=embedded])
    PAC_POP_FLAG([LIBS])
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

dnl in src/mpid/ch3/subconfigure.m4, we somehow need double check
AC_DEFUN([PAC_CHECK_OPA_CH3], [
    # ensure that atomic primitives are available
    AC_MSG_CHECKING([for OpenPA atomic primitive availability])
    PAC_OPA_present_and_working

    if test "$openpa_present_and_working" = yes ; then
        PAC_OPA_emulated_atomics
        if test "$using_emulated_atomics" = "yes" ; then
            PAC_OPA_explicitly_emulated
            if test "$atomics_explicitly_emulated" = "yes" ; then
                AC_MSG_RESULT([yes (emulated)])
            else
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([
    The ch3 device was selected yet no native atomic primitives are
    available on this platform.  OpenPA can emulate atomic primitives using
    locks by specifying --with-atomic-primitives=no but performance will be
    very poor.  This override should only be specified for correctness
    testing purposes.])
            fi
        else
            AC_MSG_RESULT([yes])
        fi
    else
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([
    The ch3 devies was selected yet a set of working OpenPA headers
    were not found.  Please check the OpenPA configure step for errors.])
    fi
])

AC_DEFUN([PAC_OPA_present_and_working], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <opa_primitives.h> /* will include pthread.h if present and needed */
    ]],[[
        OPA_int_t a, b;
        int c;
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
        pthread_mutex_t shm_lock;
        OPA_Interprocess_lock_init(&shm_lock, 1/*isLeader*/);
#endif

        OPA_store_int(&a, 0);
        OPA_store_int(&b, 1);
        OPA_add_int(&a, 10);
        OPA_assert(10 == OPA_load_int(&a));
        c = OPA_cas_int(&a, 10, 11);
        OPA_assert(10 == c);
        c = OPA_swap_int(&a, OPA_load_int(&b));
        OPA_assert(11 == c);
        OPA_assert(1 == OPA_load_int(&a));
    ]])],
    openpa_present_and_working=yes,
    openpa_present_and_working=no)
])

AC_DEFUN([PAC_OPA_emulated_atomics],[
    AC_PREPROC_IFELSE([
    AC_LANG_SOURCE([
#include <opa_primitives.h>
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
#error "lock-based emulation is currently in use"
#endif
    ])
    ],using_emulated_atomics=no,using_emulated_atomics=yes)
])

AC_DEFUN([PAC_OPA_explicitly_emulated],[
    AC_PREPROC_IFELSE([
    AC_LANG_SOURCE([
#include <opa_primitives.h>
    /* may also be undefined in older (pre-r106) versions of OPA */
#if !defined(OPA_EXPLICIT_EMULATION)
#error "lock-based emulation was automatic, not explicit"
#endif
    ])
    ],[atomics_explicitly_emulated=yes],[atomics_explicitly_emulated=no])
])
