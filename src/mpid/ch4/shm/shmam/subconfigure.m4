[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/shm
dnl MPICH_SUBCFG_AFTER=src/mpid/ch4

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH4],[
        for shm in $ch4_shm ; do
            AS_CASE([$shm],[shmam],[build_ch4_shm_shmam=yes])
        done

        AC_ARG_WITH(ch4-shmam-eager-modules,
        [  --with-ch4-shmam-eager-modules=module-list
        CH4 SHMAM eager arguments:
                fbox - Use Fast Box module for eager transport
                ],
                [shmam_eager_modules=$withval],
                [shmam_eager_modules=])

        if test -z "${shmam_eager_modules}" ; then
            ch4_shmam_eager_modules="fbox"
        else
            ch4_shmam_eager_modules=`echo ${shmam_eager_modules} | sed -e 's/,/ /g'`
        fi

        export ch4_shmam_eager_modules

        ch4_shmam_eager_func_decl=""
        ch4_shmam_eager_func_array=""
        ch4_shmam_eager_strings=""
        ch4_shmam_eager_pre_include=""
        ch4_shmam_eager_recv_transaction_decl=""

        shmam_eager_index=0

        echo "Parsing SHMAM eager arguments"

        for shmam_eager in $ch4_shmam_eager_modules; do

            if test ! -d $srcdir/src/mpid/ch4/shm/shmam/eager/${shmam_eager} ; then
                AC_MSG_ERROR([SHMAM eager module ${shmam_eager} is unknown "$srcdir/src/mpid/ch4/shm/shmam/eager/${shmam_eager}"])
            fi

            shmam_eager_macro=`echo $shmam_eager | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
            shmam_eager_macro="MPIDI_SHMAM_${shmam_eager_macro}"

            if test -z "$ch4_shmam_eager_array" ; then
                ch4_shmam_eager_array="$shmam_eager_macro"
            else
                ch4_shmam_eager_array="$ch4_shmam_eager_array, $shmam_eager_macro"
            fi

            if test -z "$ch4_shmam_eager_func_decl" ; then
                ch4_shmam_eager_func_decl="MPIDI_SHMAM_eager_${shmam_eager}_funcs"
            else
                ch4_shmam_eager_func_decl="${ch4_shmam_eager_func_decl}, MPIDI_SHMAM_eager_${shmam_eager}_funcs"
            fi

            if test -z "$ch4_shmam_eager_func_array" ; then
                ch4_shmam_eager_func_array="&MPIDI_SHMAM_eager_${shmam_eager}_funcs"
            else
                ch4_shmam_eager_func_array="${ch4_shmam_eager_func_array}, &MPIDI_SHMAM_eager_${shmam_eager}_funcs"
            fi

            if test -z "$ch4_shmam_eager_strings" ; then
                ch4_shmam_eager_strings="\"$shmam_eager\""
            else
                ch4_shmam_eager_strings="$ch4_shmam_eager_strings, \"$shmam_eager\""
            fi

            if test -z "$ch4_shmam_eager_pre_include" ; then
                ch4_shmam_eager_pre_include="#include \"../${shmam_eager}/${shmam_eager}_pre.h\""
            else
                ch4_shmam_eager_pre_include="${ch4_shmam_eager_pre_include}
#include \"../${shmam_eager}/${shmam_eager}_pre.h\""
            fi

            if test -z "$ch4_shmam_eager_recv_transaction_decl" ; then
                ch4_shmam_eager_recv_transaction_decl="MPIDI_SHMAM_eager_${shmam_eager}_recv_transaction_t ${shmam_eager};"
            else
                ch4_shmam_eager_recv_transaction_decl="${ch4_shmam_eager_recv_transaction_decl} \\
MPIDI_SHMAM_eager_${shmam_eager}_recv_transaction_t ${shmam_eager};"
            fi

            shmam_eager_index=`expr $shmam_eager_index + 1`

        done

        ch4_shmam_eager_array_sz=$shmam_eager_index

        AC_SUBST(ch4_shmam_eager_modules)
        AC_SUBST(ch4_shmam_eager_array)
        AC_SUBST(ch4_shmam_eager_array_sz)
        AC_SUBST(ch4_shmam_eager_strings)
        AC_SUBST(ch4_shmam_eager_func_decl)
        AC_SUBST(ch4_shmam_eager_func_array)
        AC_SUBST(ch4_shmam_eager_pre_include)
        AC_SUBST(ch4_shmam_eager_recv_transaction_decl)

        if test "$ch4_shmam_eager_array_sz" = "1" ;  then
            PAC_APPEND_FLAG([-DSHMAM_EAGER_DIRECT=__shmam_eager_direct_${ch4_shmam_eager_modules}__], [CPPFLAGS])
        fi

        AC_CONFIG_FILES([
                src/mpid/ch4/shm/shmam/shmam_eager_array.c
                src/mpid/ch4/shm/shmam/eager/include/shmam_eager_pre.h
        ])

        # the SHMAM shmmod depends on the common shm code
        build_mpid_common_shm=yes
    ])
    AM_CONDITIONAL([BUILD_SHM_SHMAM],[test "X$build_ch4_shm_shmam" = "Xyes"])
])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_SHM_SHMAM],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch4:shm:shmam])
])dnl end AM_COND_IF(BUILD_SHM_SHMAM,...)
])dnl end _BODY

[#] end of __file__
