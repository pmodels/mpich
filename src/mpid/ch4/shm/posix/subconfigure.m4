[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/shm
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[posix],[build_ch4_shm_posix=yes])
        done

        AC_ARG_WITH(ch4-shmmod-posix-args,
        [  --with-ch4-shmmod-posix-args=arg1:arg2:arg3
        CH4 POSIX shmmod arguments:
                disable-lock-free-queues - Disable atomics and lock-free queues
                ],
                [posix_shmmod_args=$withval],
                [posix_shmmod_args=])

dnl Parse the shmmod arguments
        SAVE_IFS=$IFS
        IFS=':'
        args_array=$posix_shmmod_args
        do_disable_lock_free_queues=false
        echo "Parsing Arguments for POSIX shmmod"
        for arg in $args_array; do
        case ${arg} in
            disable-lock-free-queues)
                do_disable_lock_free_queues=true
                echo " ---> CH4::SHM::POSIX : $arg"
                ;;
            esac
        done
        IFS=$SAVE_IFS

        if [test "$do_disable_lock_free_queues" = "true"]; then
            AC_MSG_NOTICE([Disabling POSIX shared memory lock free queues])
        else
            AC_MSG_NOTICE([Enabling POSIX shared memory lock free queues])
            PAC_APPEND_FLAG([-DMPIDI_POSIX_USE_LOCK_FREE_QUEUES],[CPPFLAGS])
        fi
        # the POSIX shmmod depends on the common shm code
        build_mpid_common_shm=yes
    ])
    AM_CONDITIONAL([BUILD_SHM_POSIX],[test "X$build_ch4_shm_posix" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_SHM_POSIX],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:posix])
])dnl end AM_COND_IF(BUILD_SHM_POSIX,...)
])dnl end _BODY

[#] end of __file__
