[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/bc

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH4],[test "$device_name" = "ch4"])

AM_COND_IF([BUILD_CH4],[
AC_MSG_NOTICE([RUNNING PREREQ FOR CH4 DEVICE])

# the CH4 device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes
build_mpid_common_bc=yes

MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
MPID_MAX_PROCESSOR_NAME=128
MPID_MAX_ERROR_STRING=512

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

    if test -z "$ch4_netmod_coll_globals_default" ; then
        ch4_netmod_coll_globals_default="#include \"../netmod/${net}/${net}_coll_globals_default.c\""
    else
        ch4_netmod_coll_globals_default="${ch4_netmod_coll_globals_default}
#include \"../netmod/${net}/${net}_coll_globals_default.c\""
    fi

    if test -z "$ch4_netmod_pre_include" ; then
        ch4_netmod_pre_include="#include \"../netmod/${net}/${net}_pre.h\""
    else
        ch4_netmod_pre_include="${ch4_netmod_pre_include}
#include \"../netmod/${net}/${net}_pre.h\""
    fi

    if test -z "$ch4_netmod_coll_params_include" ; then
        ch4_netmod_coll_params_include="#include \"../netmod/${net}/${net}_coll_params.h\""
    else
        ch4_netmod_coll_params_include="${ch4_netmod_coll_params_include}
#include \"../netmod/${net}/${net}_coll_params.h\""
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
    if test -z "$ch4_netmod_barrier_params_decl" ; then
        ch4_netmod_barrier_params_decl="MPIDI_${net_upper}_BARRIER_PARAMS_DECL;"
    else
        ch4_netmod_barrier_params_decl="${ch4_netmod_barrier_params_decl} \\
MPIDI_${net_upper}_BARRIER_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_bcast_params_decl" ; then
        ch4_netmod_bcast_params_decl="MPIDI_${net_upper}_BCAST_PARAMS_DECL;"
    else
        ch4_netmod_bcast_params_decl="${ch4_netmod_bcast_params_decl} \\
MPIDI_${net_upper}_BCAST_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_reduce_params_decl" ; then
        ch4_netmod_reduce_params_decl="MPIDI_${net_upper}_REDUCE_PARAMS_DECL;"
    else
        ch4_netmod_reduce_params_decl="${ch4_netmod_reduce_params_decl} \\
MPIDI_${net_upper}_REDUCE_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_allreduce_params_decl" ; then
        ch4_netmod_allreduce_params_decl="MPIDI_${net_upper}_ALLREDUCE_PARAMS_DECL;"
    else
        ch4_netmod_allreduce_params_decl="${ch4_netmod_allreduce_params_decl} \\
MPIDI_${net_upper}_ALLREDUCE_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_alltoall_params_decl" ; then
        ch4_netmod_alltoall_params_decl="MPIDI_${net_upper}_ALLTOALL_PARAMS_DECL;"
    else
        ch4_netmod_alltoall_params_decl="${ch4_netmod_alltoall_params_decl} \\
MPIDI_${net_upper}_ALLTOALL_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_alltoallv_params_decl" ; then
        ch4_netmod_alltoallv_params_decl="MPIDI_${net_upper}_ALLTOALLV_PARAMS_DECL;"
    else
        ch4_netmod_alltoallv_params_decl="${ch4_netmod_alltoallv_params_decl} \\
MPIDI_${net_upper}_ALLTOALLV_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_alltoallw_params_decl" ; then
        ch4_netmod_alltoallw_params_decl="MPIDI_${net_upper}_ALLTOALLW_PARAMS_DECL;"
    else
        ch4_netmod_alltoallw_params_decl="${ch4_netmod_alltoallw_params_decl} \\
MPIDI_${net_upper}_ALLTOALLW_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_allgather_params_decl" ; then
        ch4_netmod_allgather_params_decl="MPIDI_${net_upper}_ALLGATHER_PARAMS_DECL;"
    else
        ch4_netmod_allgather_params_decl="${ch4_netmod_allgather_params_decl} \\
MPIDI_${net_upper}_ALLGATHER_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_allgatherv_params_decl" ; then
        ch4_netmod_allgatherv_params_decl="MPIDI_${net_upper}_ALLGATHERV_PARAMS_DECL;"
    else
        ch4_netmod_allgatherv_params_decl="${ch4_netmod_allgatherv_params_decl} \\
MPIDI_${net_upper}_ALLGATHERV_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_gather_params_decl" ; then
        ch4_netmod_gather_params_decl="MPIDI_${net_upper}_GATHER_PARAMS_DECL;"
    else
        ch4_netmod_gather_params_decl="${ch4_netmod_gather_params_decl} \\
MPIDI_${net_upper}_GATHER_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_gatherv_params_decl" ; then
        ch4_netmod_gatherv_params_decl="MPIDI_${net_upper}_GATHERV_PARAMS_DECL;"
    else
        ch4_netmod_gatherv_params_decl="${ch4_netmod_gatherv_params_decl} \\
MPIDI_${net_upper}_GATHERV_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_scatter_params_decl" ; then
        ch4_netmod_scatter_params_decl="MPIDI_${net_upper}_SCATTER_PARAMS_DECL;"
    else
        ch4_netmod_scatter_params_decl="${ch4_netmod_scatter_params_decl} \\
MPIDI_${net_upper}_SCATTER_PARAMS_DECL"
    fi
    if test -z "$ch4_netmod_scatterv_params_decl" ; then
        ch4_netmod_scatterv_params_decl="MPIDI_${net_upper}_SCATTERV_PARAMS_DECL;"
    else
        ch4_netmod_scatterv_params_decl="${ch4_netmod_scatterv_params_decl} \\
MPIDI_${net_upper}_SCATTERV_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_reduce_scatter_params_decl" ; then
        ch4_netmod_reduce_scatter_params_decl="MPIDI_${net_upper}_REDUCE_SCATTER_PARAMS_DECL;"
    else
        ch4_netmod_reduce_scatter_params_decl="${ch4_netmod_reduce_scatter_params_decl} \\
MPIDI_${net_upper}_REDUCE_SCATTER_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_reduce_scatter_block_params_decl" ; then
        ch4_netmod_reduce_scatter_block_params_decl="MPIDI_${net_upper}_REDUCE_SCATTER_BLOCK_PARAMS_DECL;"
    else
        ch4_netmod_reduce_scatter_block_params_decl="${ch4_netmod_reduce_scatter_block_params_decl} \\
MPIDI_${net_upper}_REDUCE_SCATTER_BLOCK_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_scan_params_decl" ; then
        ch4_netmod_scan_params_decl="MPIDI_${net_upper}_SCAN_PARAMS_DECL;"
    else
        ch4_netmod_scan_params_decl="${ch4_netmod_scan_params_decl} \\
MPIDI_${net_upper}_SCAN_PARAMS_DECL;"
    fi
    if test -z "$ch4_netmod_exscan_params_decl" ; then
        ch4_netmod_exscan_params_decl="MPIDI_${net_upper}_EXSCAN_PARAMS_DECL;"
    else
        ch4_netmod_exscan_params_decl="${ch4_netmod_exscan_params_decl} \\
MPIDI_${net_upper}_EXSCAN_PARAMS_DECL;"
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
AC_SUBST(ch4_netmod_coll_globals_default)
AC_SUBST(ch4_netmod_coll_params_include)
AC_SUBST(ch4_netmod_amrequest_decl)
AC_SUBST(ch4_netmod_request_decl)
AC_SUBST(ch4_netmod_comm_decl)
AC_SUBST(ch4_netmod_dt_decl)
AC_SUBST(ch4_netmod_win_decl)
AC_SUBST(ch4_netmod_addr_decl)
AC_SUBST(ch4_netmod_op_decl)
AC_SUBST(ch4_netmod_barrier_params_decl)
AC_SUBST(ch4_netmod_bcast_params_decl)
AC_SUBST(ch4_netmod_reduce_params_decl)
AC_SUBST(ch4_netmod_allreduce_params_decl)
AC_SUBST(ch4_netmod_alltoall_params_decl)
AC_SUBST(ch4_netmod_alltoallv_params_decl)
AC_SUBST(ch4_netmod_alltoallw_params_decl)
AC_SUBST(ch4_netmod_allgather_params_decl)
AC_SUBST(ch4_netmod_allgatherv_params_decl)
AC_SUBST(ch4_netmod_gather_params_decl)
AC_SUBST(ch4_netmod_gatherv_params_decl)
AC_SUBST(ch4_netmod_scatter_params_decl)
AC_SUBST(ch4_netmod_scatterv_params_decl)
AC_SUBST(ch4_netmod_reduce_scatter_params_decl)
AC_SUBST(ch4_netmod_reduce_scatter_block_params_decl)
AC_SUBST(ch4_netmod_scan_params_decl)
AC_SUBST(ch4_netmod_exscan_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_pre_include)
AM_SUBST_NOTMAKE(ch4_netmod_coll_globals_default)
AM_SUBST_NOTMAKE(ch4_netmod_coll_params_include)
AM_SUBST_NOTMAKE(ch4_netmod_amrequest_decl)
AM_SUBST_NOTMAKE(ch4_netmod_request_decl)
AM_SUBST_NOTMAKE(ch4_netmod_comm_decl)
AM_SUBST_NOTMAKE(ch4_netmod_dt_decl)
AM_SUBST_NOTMAKE(ch4_netmod_win_decl)
AM_SUBST_NOTMAKE(ch4_netmod_addr_decl)
AM_SUBST_NOTMAKE(ch4_netmod_op_decl)
AM_SUBST_NOTMAKE(ch4_netmod_barrier_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_bcast_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_reduce_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_allreduce_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_alltoall_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_alltoallv_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_alltoallw_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_allgather_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_allgatherv_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_gather_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_gatherv_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_scatter_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_scatterv_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_reduce_scatter_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_reduce_scatter_block_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_scan_params_decl)
AM_SUBST_NOTMAKE(ch4_netmod_exscan_params_decl)

AC_ARG_ENABLE(ch4-netmod-inline,
    [--enable-ch4-netmod-inline
       Enables inlined netmod build when a single netmod is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,enable_ch4_netmod_inline=yes)


AC_ARG_ENABLE(ch4-netmod-direct,
    [--enable-ch4-netmod-direct
       (Deprecated in favor of ch4-netmod-inline)
       Enables inlined netmod build when a single netmod is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,)

if test "$ch4_nets_array_sz" = "1" && (test "$enable_ch4_netmod_inline" = "yes" || test "$enable_ch4_netmod_direct" = "yes") ;  then
   PAC_APPEND_FLAG([-DNETMOD_INLINE=__netmod_inline_${ch4_netmods}__], [CPPFLAGS])
fi

AC_ARG_ENABLE(ch4-direct,
    [--enable-ch4-direct
       Defines the direct communication routine used in CH4 device
       level:
         netmod     - Directly transfer data through the chosen netmode
         auto       - The CH4 device controls whether transfer data through netmod
                      or through shared memory based on locality
    ],,enable_ch4_direct=default)

AC_ARG_ENABLE(ch4-shm-inline,
    [--enable-ch4-shm-inline
       Enables inlined shared memory build when a single shared memory module is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,enable_ch4_shm_inline=yes)

AC_ARG_ENABLE(ch4-shm-direct,
    [--enable-ch4-shm-direct
       (Deprecated in favor of ch4-shm-inline)
       Enables inlined shared memory build when a single shared memory module is used
       level:
         yes       - Enabled (default)
         no        - Disabled (may improve build times and code size)
    ],,)

if test "$enable_ch4_shm_inline" = "yes" || test "$enable_ch4_shm_direct" = "yes" ;  then
   PAC_APPEND_FLAG([-DSHM_INLINE=__shm_inline_${ch4_shm}__], [CPPFLAGS])
fi

# setup shared memory defaults
# TODO: shm submodules should be chosen with similar configure option as that used for netmod.
# We can add it when a shm submodule is added. Now we just simply set POSIX.
ch4_shm=posix
export ch4_shm

# setup default direct communication routine
if test "${enable_ch4_direct}" = "default" ; then
    # ucx can only choose direct netmod because it does not handle any_src
    # receive when both nemod and shared memory are used.
    if test "${ch4_netmods}" = "ucx" ; then
        enable_ch4_direct=netmod
    else
        enable_ch4_direct=auto
    fi
fi

if test "$enable_ch4_direct" != "auto" -a "$enable_ch4_direct" != "netmod"; then
    AC_MSG_ERROR([Direct comunication option ${enable_ch4_direct} is unknown])
fi

if test "$enable_ch4_direct" = "auto" ; then
    # This variable can be set either when CH4 controls the data transfer routine
    # or when the netmod doesn't want to implement its own locality information
    AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
elif test "$enable_ch4_direct" = "netmod" ; then
    AC_DEFINE(MPIDI_CH4_DIRECT_NETMOD, 1, [CH4 Directly transfers data through the chosen netmode])
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

AC_ARG_ENABLE(ch4-mt,
    [--enable-ch4-mt=model
       Select model for multi-threading
         direct    - Each thread directly accesses lower-level fabric (default)
         handoff   - Use the hand-off model (spawns progress thread)
         trylock   - Use the trylock-enqueue model
         runtime   - Determine the model at runtime through a CVAR
    ],,enable_ch4_mt=direct)

case $enable_ch4_mt in
     direct)
         AC_DEFINE([MPIDI_CH4_USE_MT_DIRECT], [1],
            [Define to enable direct multi-threading model])
        ;;
     handoff)
         AC_DEFINE([MPIDI_CH4_USE_MT_HANDOFF], [1],
            [Define to enable hand-off multi-threading model])
        ;;
     trylock)
         AC_DEFINE([MPIDI_CH4_USE_MT_TRYLOCK], [1],
            [Define to enable trylock-enqueue multi-threading model])
        ;;
     runtime)
         AC_DEFINE([MPIDI_CH4_USE_MT_RUNTIME], [1],
            [Define to enable runtime multi-threading model])
        ;;
     *)
        AC_MSG_ERROR([Multi-threading model ${enable_ch4_mt} is unknown])
        ;;
esac

#
# Dependency checks for CH4 MT modes
# Currently, "handoff", "trylock", and "runtime" require the followings:
# - izem linked in (--with-zm-prefix)
# - enable-thread-cs=per-vni
#
if test "$enable_ch4_mt" != "direct"; then
    if test "${with_zm_prefix}" == "no" -o "${with_zm_prefix}" == "none" -o "${izem_queue}" != "yes" ; then
        AC_MSG_ERROR([Multi-threading model `${enable_ch4_mt}` requires izem queue. Set `--enable-izem={queue|all} --with-zm-prefix` and retry.])
    elif test "${enable_thread_cs}" != "per-vni" -a "${enable_thread_cs}" != "per_vni"; then
        AC_MSG_ERROR([Multi-threading model `${enable_ch4_mt}` requires `--enable-thread-cs=per-vni`.])
    fi
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

# make sure we support signal
AC_CHECK_HEADERS(signal.h)
AC_CHECK_FUNCS(signal)

AC_CONFIG_FILES([
src/mpid/ch4/src/mpid_ch4_net_array.c
src/mpid/ch4/include/netmodpre.h
src/mpid/ch4/include/coll_algo_params.h
src/mpid/ch4/src/ch4_coll_globals_default.c
])
])dnl end AM_COND_IF(BUILD_CH4,...)

AM_CONDITIONAL([BUILD_CH4_COLL_TUNING],[test -e "$srcdir/src/mpid/ch4/src/ch4_coll_globals.c"])

])dnl end _BODY

[#] end of __file__
