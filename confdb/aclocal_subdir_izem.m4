AC_DEFUN([PAC_SUBDIR_ENABLE_IZEM],[
    AC_ARG_ENABLE([izem],
        [  --enable-izem@<:@=ARGS@:>@   Enable features from the Izem library.
                            "ARGS" is a list of comma separated features.
                            Accepted arguments are:
                            sync     - use the Izem interface for sychronization objects
                                        (locks and condition variables) instead of the MPL interface
                            queue    - use the Izem interface for atomic queue objects
                            atomic   - use the Izem interface for CPU atomics
                            yes/all  - all of the above features are enabled
                            no/none  - none of the above features are enabled],
        [],
        [enable_izem=no])

    # strip off multiple options, separated by commas
    save_IFS="$IFS"
    IFS=","
    for option in $enable_izem ; do
        case "$option" in
            sync)
                izem_sync=yes
            ;;
            queue)
                izem_queue=yes
            ;;
            atomic)
                izem_atomic=yes
            ;;
            yes|all)
                izem_sync=yes
                izem_queue=yes
                izem_atomic=yes
            ;;
            no|none)
            ;;
            *)
                AC_MSG_ERROR([Unknown value $option for enable-izem])
            ;;
        esac
    done
    IFS="$save_IFS"

    if test "$izem_sync" = "yes" ; then
        AC_DEFINE(ENABLE_IZEM_SYNC,1,[Define to enable using Izem locks and condition variables])
    fi

    if test "$izem_queue" = "yes" ; then
        AC_DEFINE(ENABLE_IZEM_QUEUE,1,[Define to enable using Izem queues])
    fi

    if test "$izem_atomic" = "yes" ; then
        AC_DEFINE(ENABLE_IZEM_ATOMIC,1,[Define to enable using Izem CPU atomics])
    fi
])

AC_DEFUN([PAC_SUBDIR_IZEM],[
    PAC_SUBDIR_LIBNAME([ZMLIBNAME],[zm])
    AC_ARG_WITH([zm-prefix],
    [  --with-zm-prefix@<:@=ARG@:>@
                            specify the Izem library to use. No argument implies "yes".
                            Accepted values for ARG are:
                            yes|embedded - use the embedded Izem
                            system       - search system paths for an Izem installation
                            no           - disable Izem
                            <PATH>       - use the Izem at PATH],,
    [with_zm_prefix=no])
    if test "$enable_izem" != "no" && test "$enable_izem" != "none"; then
        if test "$with_zm_prefix" = "yes" || test "$with_zm_prefix" = "embedded"; then
            PAC_SUBDIR_EMBED([$ZMLIBNAME],[$1],[$1/src],[$1/src])
        elif test "$with_zm_prefix" = "system"; then
            PAC_SUBDIR_TEST_SYSTEM_IZEM
            PAC_SUBDIR_SYSTEM([$ZMLIBNAME])
        elif test "$with_zm_prefix" = "no"; then
            AC_MSG_ERROR([Izem features were requested with --enable-izem but Izem was disabled.])
        else
            PAC_SUBDIR_PREFIX_CHECK([Izem],[$with_zm_prefix],[include/lock/zm_lock.h])
            PAC_SUBDIR_PREFIX_CHECK([Izem],[$with_zm_prefix],[include/cond/zm_cond.h])
            PAC_SUBDIR_PREFIX([$ZMLIBNAME],[$with_zm_prefix])
            PAC_SUBDIR_TEST_PREFIX_IZEM
        fi
    fi
])

AC_DEFUN([PAC_SUBDIR_TEST_SYSTEM_IZEM],[
    PAC_PUSH_FLAG([LIBS])
    PAC_PREPEND_FLAG([-l${ZMLIBNAME}],[LIBS])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([dnl
        #include "lock/zm_ticket.h"
        ], [
        zm_ticket_t lock;
        zm_ticket_init(&lock);
        zm_ticket_acquire(&lock);
        zm_ticket_release(&lock);])dnl
        ],
        [AC_MSG_ERROR([No usable Izem installation was found on this system])],
        [])
    PAC_POP_FLAG([LIBS])
])

AC_DEFUN([PAC_SUBDIR_TEST_PREFIX_IZEM],[
    PAC_PUSH_FLAG([LIBS])
    PAC_PUSH_FLAG([LDFLAGS])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "lock/zm_ticket.h"
                                    ],
                                    [zm_ticket_t lock;
                                        zm_ticket_init(&lock);
                                        zm_ticket_acquire(&lock);
                                        zm_ticket_release(&lock);])],
                    [],[AC_MSG_ERROR([The Izem installation at "${with_zm_prefix}" seems broken])])
    PAC_POP_FLAG([LIBS])
    PAC_POP_FLAG([LDFLAGS])
])

AC_DEFUN([PAC_CONFIG_ARG_IZEM],[
    config_arg="--enable-embedded"
])

