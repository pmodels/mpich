[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH4],[test "$device_name" = "ch4"])

# the CH4 device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes

MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
MPID_MAX_PROCESSOR_NAME=128
MPID_MAX_ERROR_STRING=512

AM_COND_IF([BUILD_CH4],[
AC_MSG_NOTICE([RUNNING PREREQ FOR CH4 DEVICE])

# $device_args - contains the netmods
if test -z "${device_args}" ; then
    ch4_netmods="ofi"
else
    changequote(<<,>>)
    netmod_args=`echo ${device_args} | sed -e 's/^[^:]*//' -e 's/^://' -e 's/,/ /g'`
    changequote([,])
    ch4_netmods=`echo ${device_args} | sed -e 's/:.*$//' -e 's/,/ /g'`
fi
export ch4_netmods
export netmod_args

#
# reset DEVICE so that it (a) always includes the channel name, and (b) does not include channel options
#
DEVICE="${device_name}:${ch4_netmods}"

ch4_nets_func_decl=""
ch4_nets_native_func_decl=""
ch4_nets_func_array=""
ch4_nets_native_func_array=""
ch4_nets_strings=""
net_index=0
for net in $ch4_netmods ; do
    if test ! -d $srcdir/src/mpid/ch4/netmod/${net} ; then
        AC_MSG_ERROR([Network module ${net} is unknown "$srcdir/src/mpid/ch4/netmod/${net}"])
    fi
    net_macro=`echo $net | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    net_macro="MPIDI_CH4_${net_macro}"

    if test -z "$ch4_nets_array" ; then
        ch4_nets_array="$net_macro"
    else
        ch4_nets_array="$ch4_nets_array, $net_macro"
    fi

    if test -z "$ch4_nets_func_decl" ; then
        ch4_nets_func_decl="MPIDI_NM_${net}_funcs"
    else
        ch4_nets_func_decl="${ch4_nets_func_decl}, MPIDI_NM_${net}_funcs"
    fi

    if test -z "$ch4_nets_native_func_decl" ; then
        ch4_nets_native_func_decl="MPIDI_NM_native_${net}_funcs"
    else
        ch4_nets_native_func_decl="${ch4_nets_native_func_decl}, MPIDI_NM_native_${net}_funcs"
    fi

    if test -z "$ch4_nets_func_array" ; then
        ch4_nets_func_array="&MPIDI_NM_${net}_funcs"
    else
        ch4_nets_func_array="${ch4_nets_func_array}, &MPIDI_NM_${net}_funcs"
    fi

    if test -z "$ch4_nets_native_func_array" ; then
        ch4_nets_native_func_array="&MPIDI_NM_native_${net}_funcs"
    else
        ch4_nets_native_func_array="${ch4_nets_native_func_array}, &MPIDI_NM_native_${net}_funcs"
    fi

    if test -z "$ch4_nets_strings" ; then
        ch4_nets_strings="\"$net\""
    else
        ch4_nets_strings="$ch4_nets_strings, \"$net\""
    fi

    if test -z "$ch4_netmod_pre_include" ; then
        ch4_netmod_pre_include="#include \"../netmod/${net}/${net}_pre.h\""
    else
        ch4_netmod_pre_include="${ch4_netmod_pre_include}
#include \"../netmod/${net}/${net}_pre.h\""
    fi

    net_upper=`echo ${net} | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    if test -z "$ch4_netmod_amrequest_decl" ; then
        ch4_netmod_amrequest_decl="MPIDI_${net_upper}_am_request_t ${net};"
    else
        ch4_netmod_amrequest_decl="${ch4_netmod_amrequest_decl} \\
MPIDI_${net_upper}_am_request_t ${net};"
    fi

    if test -z "$ch4_netmod_request_decl" ; then
        ch4_netmod_request_decl="MPIDI_${net_upper}_request_t ${net};"
    else
        ch4_netmod_request_decl="${ch4_netmod_request_decl} \\
MPIDI_${net_upper}_request_t ${net};"
    fi

    if test -z "$ch4_netmod_comm_decl" ; then
        ch4_netmod_comm_decl="MPIDI_${net_upper}_comm_t ${net};"
    else
        ch4_netmod_comm_decl="${ch4_netmod_comm_decl} \\
MPIDI_${net_upper}_comm_t ${net};"
    fi
    if test -z "$ch4_netmod_dt_decl" ; then
        ch4_netmod_dt_decl="MPIDI_${net_upper}_dt_t ${net};"
    else
        ch4_netmod_dt_decl="${ch4_netmod_dt_decl} \\
MPIDI_${net_upper}_dt_t ${net};"
    fi
    if test -z "$ch4_netmod_op_decl" ; then
        ch4_netmod_op_decl="MPIDI_${net_upper}_op_t ${net};"
    else
        ch4_netmod_op_decl="${ch4_netmod_op_decl} \\
MPIDI_${net_upper}_op_t ${net};"
    fi

    if test -z "$ch4_netmod_win_decl" ; then
        ch4_netmod_win_decl="MPIDI_${net_upper}_win_t ${net};"
    else
        ch4_netmod_win_decl="${ch4_netmod_win_decl} \\
MPIDI_${net_upper}_win_t ${net};"
    fi
    if test -z "$ch4_netmod_addr_decl" ; then
        ch4_netmod_addr_decl="MPIDI_${net_upper}_addr_t ${net};"
    else
        ch4_netmod_addr_decl="${ch4_netmod_addr_decl} \\
MPIDI_${net_upper}_addr_t ${net};"
    fi




net_index=`expr $net_index + 1`
done
ch4_nets_array_sz=$net_index

AC_SUBST(device_name)
AC_SUBST(ch4_netmods)
AC_SUBST(ch4_nets_array)
AC_SUBST(ch4_nets_array_sz)
AC_SUBST(ch4_nets_func_decl)
AC_SUBST(ch4_nets_native_func_decl)
AC_SUBST(ch4_nets_func_array)
AC_SUBST(ch4_nets_native_func_array)
AC_SUBST(ch4_nets_strings)
AC_SUBST(ch4_netmod_pre_include)
AC_SUBST(ch4_netmod_amrequest_decl)
AC_SUBST(ch4_netmod_request_decl)
AC_SUBST(ch4_netmod_comm_decl)
AC_SUBST(ch4_netmod_dt_decl)
AC_SUBST(ch4_netmod_win_decl)
AC_SUBST(ch4_netmod_addr_decl)
AC_SUBST(ch4_netmod_op_decl)
AM_SUBST_NOTMAKE(ch4_netmod_pre_include)
AM_SUBST_NOTMAKE(ch4_netmod_amrequest_decl)
AM_SUBST_NOTMAKE(ch4_netmod_request_decl)
AM_SUBST_NOTMAKE(ch4_netmod_comm_decl)
AM_SUBST_NOTMAKE(ch4_netmod_dt_decl)
AM_SUBST_NOTMAKE(ch4_netmod_win_decl)
AM_SUBST_NOTMAKE(ch4_netmod_addr_decl)
AM_SUBST_NOTMAKE(ch4_netmod_op_decl)

AC_ARG_ENABLE(ch4-netmod-direct,
    [--enable-ch4-netmod-direct
       Enables inlined netmod build when a single netmod is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,enable_ch4_netmod_direct=yes)


if test "$ch4_nets_array_sz" = "1" && test "$enable_ch4_netmod_direct" = "yes" ;  then
   PAC_APPEND_FLAG([-DNETMOD_DIRECT=__netmod_direct_${ch4_netmods}__], [CPPFLAGS])
fi


AC_ARG_ENABLE(ch4-shm,
    [--enable-ch4-shm=level:module
       Control whether CH4 shared memory is built and/or used. Default
       shm level depends on selected netmod(s). (OFI=exclusive, UCX=no).
       level:
         no        - Do not build or use CH4 shared memory.
         yes       - Build CH4 shared memory, but do not use it by default (Your chosen netmod must provide it).
         exclusive - Build and exclusively use CH4 shared memory.
       module-list(optional).  comma separated list of shared memory modules:
         posix     - POSIX shared memory implementation
    ],,enable_ch4_shm=default)

AC_ARG_ENABLE(ch4-shm-direct,
    [--enable-ch4-shm-direct
       Enables inlined shared memory build when a single shared memory module is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,enable_ch4_shm_direct=yes)

# setup shared memory defaults
if test "${enable_ch4_shm}" = "default" ; then
    if test "${ch4_netmods}" = "ucx" ; then
        enable_ch4_shm=no
    else
	enable_ch4_shm=exclusive:posix
    fi
fi

ch4_shm_level=`echo $enable_ch4_shm | sed -e 's/:.*$//'`
changequote(<<,>>)
ch4_shm=`echo $enable_ch4_shm | sed -e 's/^[^:]*//' -e 's/^://'`
changequote([,])

if test "$ch4_shm_level" != "no" -a "$ch4_shm_level" != "yes" -a "$ch4_shm_level" != "exclusive"; then
    AC_MSG_ERROR([Shared memory level ${ch4_shm_level} is unknown])
fi

if test "$ch4_shm_level" != "no" ; then
    AC_DEFINE([MPIDI_BUILD_CH4_SHM], [1],
        [Define if CH4 will build the default shared memory implementation as opposed to only using a netmod implementation])
fi

if test "$ch4_shm_level" = "exclusive" ; then
    # This variable is set only when the user wants CH4 to handle all shared memory operations
    AC_DEFINE(MPIDI_CH4_EXCLUSIVE_SHM, 1, [Define if CH4 will be providing the exclusive implementation of shared memory])

    # This variable can be set either when the user asks for CH4 exclusive shared memory
    # or when the netmod doesn't want to implement its own locality information
    AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
fi

# $ch4_shm - contains the shm mods
if test -z "${ch4_shm}" ; then
   if test "$ch4_shm_level" != "no" ; then
      ch4_shm="posix"
   fi
else
   ch4_shm=`echo ${ch4_shm} | sed -e 's/,/ /g'`
fi
export ch4_shm

ch4_shm_func_decl=""
ch4_shm_native_func_decl=""
ch4_shm_func_array=""
ch4_shm_native_func_array=""
ch4_shm_strings=""
shm_index=0
for shm in $ch4_shm ; do
    if test ! -d $srcdir/src/mpid/ch4/shm/${shm} ; then
        AC_MSG_ERROR([Shared memory module ${shm} is unknown "$srcdir/src/mpid/ch4/shm/${shm}"])
    fi
    shm_macro=`echo $shm | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    shm_macro="MPIDI_SHM_${shm_macro}"

    if test -z "$ch4_shm_array" ; then
        ch4_shm_array="$shm_macro"
    else
        ch4_shm_array="$ch4_shm_array, $shm_macro"
    fi

    if test -z "$ch4_shm_func_decl" ; then
        ch4_shm_func_decl="MPIDI_SHM_${shm}_funcs"
    else
        ch4_shm_func_decl="${ch4_shm_func_decl}, MPIDI_SHM_${shm}_funcs"
    fi

    if test -z "$ch4_shm_native_func_decl" ; then
        ch4_shm_native_func_decl="MPIDI_SHM_native_${shm}_funcs"
    else
        ch4_shm_native_func_decl="${ch4_shm_native_func_decl}, MPIDI_SHM_native_${shm}_funcs"
    fi

    if test -z "$ch4_shm_func_array" ; then
        ch4_shm_func_array="&MPIDI_SHM_${shm}_funcs"
    else
        ch4_shm_func_array="${ch4_shm_func_array}, &MPIDI_SHM_${shm}_funcs"
    fi

    if test -z "$ch4_shm_native_func_array" ; then
        ch4_shm_native_func_array="&MPIDI_SHM_native_${shm}_funcs"
    else
        ch4_shm_native_func_array="${ch4_shm_native_func_array}, &MPIDI_SHM_native_${shm}_funcs"
    fi

    if test -z "$ch4_shm_strings" ; then
        ch4_shm_strings="\"$shm\""
    else
        ch4_shm_strings="$ch4_shm_strings, \"$shm\""
    fi

    if test -z "$ch4_shm_pre_include" ; then
        ch4_shm_pre_include="#include \"../shm/${shm}/${shm}_pre.h\""
    else
        ch4_shm_pre_include="${ch4_shm_pre_include}
#include \"../shm/${shm}/${shm}_pre.h\""
    fi

    shm_upper=`echo ${shm} | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    if test -z "$ch4_shm_request_decl" ; then
        ch4_shm_request_decl="MPIDI_${shm_upper}_request_t ${shm};"
    else
        ch4_shm_request_decl="${ch4_shm_request_decl} \\
MPIDI_${shm_upper}_request_t ${shm};"
    fi

    if test -z "$ch4_shm_comm_decl" ; then
        ch4_shm_comm_decl="MPIDI_${shm_upper}_comm_t ${shm};"
    else
        ch4_shm_comm_decl="${ch4_shm_comm_decl} \\
MPIDI_${shm_upper}_comm_t ${shm};"
    fi


    shm_index=`expr $shm_index + 1`
done
ch4_shm_array_sz=$shm_index

AC_SUBST(ch4_shm)
AC_SUBST(ch4_shm_array)
AC_SUBST(ch4_shm_array_sz)
AC_SUBST(ch4_shm_func_decl)
AC_SUBST(ch4_shm_native_func_decl)
AC_SUBST(ch4_shm_func_array)
AC_SUBST(ch4_shm_native_func_array)
AC_SUBST(ch4_shm_strings)
AC_SUBST(ch4_shm_pre_include)
AC_SUBST(ch4_shm_request_decl)
AC_SUBST(ch4_shm_comm_decl)
AM_SUBST_NOTMAKE(ch4_shm_pre_include)
AM_SUBST_NOTMAKE(ch4_shm_request_decl)
AM_SUBST_NOTMAKE(ch4_shm_comm_decl)

if test "$ch4_shm_array_sz" = "1"  && test "$enable_ch4_shm_direct" = "yes" ;  then
   PAC_APPEND_FLAG([-DSHM_DIRECT=__shm_direct_${ch4_shm}__], [CPPFLAGS])
fi

])dnl end AM_COND_IF(BUILD_CH4,...)
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_CH4],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR CH4 DEVICE])

AC_ARG_WITH(ch4-rank-bits, [--with-ch4-rank-bits=16/32     Number of bits allocated to the rank field (16 or 32)],
			   [ rankbits=$withval ],
			   [ rankbits=32 ])
if test "$rankbits" != "16" -a "$rankbits" != "32" ; then
   AC_MSG_ERROR(Only 16 or 32-bit ranks are supported)
fi
AC_DEFINE_UNQUOTED(CH4_RANK_BITS,$rankbits,[Define the number of CH4_RANK_BITS])

AC_ARG_ENABLE(ch4r-per-comm-msg-queue,
    [--enable-ch4r-per-comm-msg-queue=option
       Enable use of per-communicator message queues for posted recvs/unexpected messages
         yes       - Use per-communicator message queue. (Default)
         no        - Use global queue for posted recvs/unexpected messages.
    ],,enable_ch4r_per_comm_msg_queue=yes)

if test "$enable_ch4r_per_comm_msg_queue" = "yes" ; then
    AC_DEFINE([MPIDI_CH4U_USE_PER_COMM_QUEUE], [1],
        [Define if CH4U will use per-communicator message queues])
fi

AC_CHECK_HEADERS(sys/mman.h sys/stat.h fcntl.h)
AC_CHECK_FUNC(mmap, [], [AC_MSG_ERROR(mmap is required to build CH4)])

gl_FUNC_RANDOM_R
if test "$HAVE_RANDOM_R" = "1" -a "$HAVE_STRUCT_RANDOM_DATA" = "1" ; then
    AC_DEFINE(USE_SYM_HEAP,1,[Define if we can use a symmetric heap])
    AC_MSG_NOTICE([Using a symmetric heap])
else
    AC_MSG_NOTICE([Using a non-symmetric heap])
fi

AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

AC_CONFIG_FILES([
src/mpid/ch4/src/mpid_ch4_net_array.c
src/mpid/ch4/include/netmodpre.h
src/mpid/ch4/include/shmpre.h
src/mpid/ch4/include/coll_algo_params.h
])
])dnl end AM_COND_IF(BUILD_CH4,...)

AM_CONDITIONAL([BUILD_CH4_SHM],[test "$ch4_shm_level" = "yes" -o "$ch4_shm_level" = "exclusive"])
AM_CONDITIONAL([BUILD_CH4_COLL_TUNING],[test -e "$srcdir/src/mpid/ch4/src/ch4_coll_globals.c"])

])dnl end _BODY

[#] end of __file__
