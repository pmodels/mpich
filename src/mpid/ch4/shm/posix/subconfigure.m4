[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/shm
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[posix],[build_ch4_shm_posix=yes])
        done

        AC_ARG_WITH(ch4-posix-eager-modules,
        [  --with-ch4-posix-eager-modules=module-list
        CH4 POSIX eager arguments:
                fbox - Use Fast Box module for eager transport
                iqueue - Use Inverted Queue module for eager transport
                ],
                [posix_eager_modules=$withval],
                [posix_eager_modules=])

        if test -z "${posix_eager_modules}" ; then
            ch4_posix_eager_modules="fbox"
        else
            ch4_posix_eager_modules=`echo ${posix_eager_modules} | sed -e 's/,/ /g'`
        fi

        export ch4_posix_eager_modules

        ch4_posix_eager_func_decl=""
        ch4_posix_eager_func_array=""
        ch4_posix_eager_strings=""
        ch4_posix_eager_pre_include=""
        ch4_posix_eager_recv_transaction_decl=""

        posix_eager_index=0

        echo "Parsing POSIX eager arguments"

        for posix_eager in $ch4_posix_eager_modules; do

            if test ! -d $srcdir/src/mpid/ch4/shm/posix/eager/${posix_eager} ; then
                AC_MSG_ERROR([POSIX eager module ${posix_eager} is unknown "$srcdir/src/mpid/ch4/shm/posix/eager/${posix_eager}"])
            fi

            posix_eager_macro=`echo $posix_eager | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
            posix_eager_macro="MPIDI_POSIX_${posix_eager_macro}"

            if test -z "$ch4_posix_eager_array" ; then
                ch4_posix_eager_array="$posix_eager_macro"
            else
                ch4_posix_eager_array="$ch4_posix_eager_array, $posix_eager_macro"
            fi

            if test -z "$ch4_posix_eager_func_decl" ; then
                ch4_posix_eager_func_decl="MPIDI_POSIX_eager_${posix_eager}_funcs"
            else
                ch4_posix_eager_func_decl="${ch4_posix_eager_func_decl}, MPIDI_POSIX_eager_${posix_eager}_funcs"
            fi

            if test -z "$ch4_posix_eager_func_array" ; then
                ch4_posix_eager_func_array="&MPIDI_POSIX_eager_${posix_eager}_funcs"
            else
                ch4_posix_eager_func_array="${ch4_posix_eager_func_array}, &MPIDI_POSIX_eager_${posix_eager}_funcs"
            fi

            if test -z "$ch4_posix_eager_strings" ; then
                ch4_posix_eager_strings="\"$posix_eager\""
            else
                ch4_posix_eager_strings="$ch4_posix_eager_strings, \"$posix_eager\""
            fi

            if test -z "$ch4_posix_eager_pre_include" ; then
                ch4_posix_eager_pre_include="#include \"../${posix_eager}/${posix_eager}_pre.h\""
            else
                ch4_posix_eager_pre_include="${ch4_posix_eager_pre_include}
#include \"../${posix_eager}/${posix_eager}_pre.h\""
            fi

            if test -z "$ch4_posix_eager_recv_transaction_decl" ; then
                ch4_posix_eager_recv_transaction_decl="MPIDI_POSIX_eager_${posix_eager}_recv_transaction_t ${posix_eager};"
            else
                ch4_posix_eager_recv_transaction_decl="${ch4_posix_eager_recv_transaction_decl} \\
MPIDI_POSIX_eager_${posix_eager}_recv_transaction_t ${posix_eager};"
            fi

            posix_eager_index=`expr $posix_eager_index + 1`

        done

        ch4_posix_eager_array_sz=$posix_eager_index

        echo "There are $ch4_posix_eager_array_sz POSIX eager modules"

        AC_SUBST(ch4_posix_eager_modules)
        AC_SUBST(ch4_posix_eager_array)
        AC_SUBST(ch4_posix_eager_array_sz)
        AC_SUBST(ch4_posix_eager_strings)
        AC_SUBST(ch4_posix_eager_func_decl)
        AC_SUBST(ch4_posix_eager_func_array)
        AC_SUBST(ch4_posix_eager_pre_include)
        AC_SUBST(ch4_posix_eager_recv_transaction_decl)

        if test "$ch4_posix_eager_array_sz" = "1" ;  then
            PAC_APPEND_FLAG([-DPOSIX_EAGER_DIRECT=__posix_eager_direct_${ch4_posix_eager_modules}__], [CPPFLAGS])
        fi

        AC_CONFIG_FILES([
                src/mpid/ch4/shm/posix/posix_eager_array.c
                src/mpid/ch4/shm/posix/eager/include/posix_eager_pre.h
        ])

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
