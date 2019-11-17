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
    dnl PAC_SUBDIR_LIBNAME([ZMLIBNAME],[zm])
    PAC_SUBDIR_CONFIG([zm],[lock/zm_ticket.h],[zm],[zm_ticket_init])
    PAC_SUBDIR_CONFIG_EMBED_OPTIONAL([zm],[zm],[$1],[src],[src])
])

AC_DEFUN([PAC_CONFIG_ARG_IZEM],[
    config_arg="--enable-embedded"
])

