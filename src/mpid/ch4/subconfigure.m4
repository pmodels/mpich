[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/bc
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/genq

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH4],[test "$device_name" = "ch4"])

AM_COND_IF([BUILD_CH4],[
AC_MSG_NOTICE([RUNNING PREREQ FOR CH4 DEVICE])

# check availability of libfabric
if test x"$with_libfabric" != x"embedded" ; then
    PAC_SET_HEADER_LIB_PATH(libfabric)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB([rdma/fabric.h], [fabric], [fi_getinfo], [have_libfabric=yes], [have_libfabric=no])
    PAC_POP_FLAG(LIBS)
else
    have_libfabric=no
fi

# check availability of ucx
if test x"$with_ucx" != x"embedded" ; then
    PAC_SET_HEADER_LIB_PATH(ucx)
    PAC_PUSH_FLAG(LIBS)
    PAC_CHECK_HEADER_LIB([ucp/api/ucp.h], [ucp], [ucp_config_read], [have_ucx=yes], [have_ucx=no])
    PAC_POP_FLAG(LIBS)
else
    have_ucx=no
fi

# the CH4 device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes
build_mpid_common_bc=yes
build_mpid_common_genq=yes

MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
MPID_MAX_PROCESSOR_NAME=128
MPID_MAX_ERROR_STRING=512

# $device_args - contains the netmods
if test -z "${device_args}" ; then
  AS_CASE([$host_os],
  [linux*],[
    dnl attempt to choose a netmod from the installed libraries
    if test $have_ucx = "yes" -a $have_libfabric = "no" ; then
        ch4_netmods=ucx
    elif test $have_ucx = "no" -a $have_libfabric = "yes" ; then
        ch4_netmods=ofi
    else
        dnl prompt the user to choose
        AC_MSG_ERROR([no ch4 netmod selected

  The default ch4 device could not detect a preferred network
  library. Supported options are ofi (libfabric) and ucx:

    --with-device=ch4:ofi or --with-device=ch4:ucx

  Configure will use an embedded copy of libfabric or ucx if one is
  not found in the user environment. An installation can be specified
  by adding

    --with-libfabric=<path/to/install> or --with-ucx=<path/to/install>

  to the configuration.

  The previous MPICH default device (ch3) is also available and
  supported with option:

    --with-device=ch3
    ])
    fi],
    [
      dnl non-linux use libfabric
      ch4_netmods=ofi
    ])
else
    changequote(<<,>>)
    netmod_args=`echo ${device_args} | sed -e 's/^[^:]*//' -e 's/^://' -e 's/,/ /g'`
    changequote([,])
    ch4_netmods=`echo ${device_args} | sed -e 's/:.*$//' -e 's/,/ /g'`
fi
export ch4_netmods
export netmod_args
AC_MSG_NOTICE([CH4 select netmod: $ch4_netmods $netmod_args])

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

AC_ARG_ENABLE([ch4-direct],
              [--enable-ch4-direct   DO NOT USE!  Use --without-ch4-shmmods instead],
              [enable_ch4_direct=yes],[enable_ch4_direct=no])
if test "${enable_ch4_direct}" = "yes" ; then
    AC_MSG_ERROR([do not use --enable-ch4-direct; use --without-ch4-shmmods instead])
fi

# setup shared memory submodules
AC_ARG_WITH(ch4-shmmods,
    [  --with-ch4-shmmods@<:@=ARG@:>@ Comma-separated list of shared memory modules for MPICH/CH4.
                          Valid options are:
                          auto         - Enable everything that is available/allowed by netmod (default)
                                         (cannot be combined with other options)
                          none         - No shmmods, network only (cannot be combined with other options)
                          posix        - Enable POSIX shmmod
                          xpmem        - Enable XPMEM IPC (requires posix)
                          gpudirect    - Enable GPU Direct IPC (requires posix)
                 ],
                 [with_ch4_shmmods=$withval],
                 [with_ch4_shmmods=auto])
# shmmod0,shmmod1,... format
# (posix is always enabled thus ch4_shm is not checked in posix module)
ch4_shm="`echo $with_ch4_shmmods | sed -e 's/,/ /g'`"
export ch4_shm

# setup default direct communication routine
if test "${with_ch4_shmmods}" = "auto" -a "${ch4_netmods}" = "ucx" ; then
    # ucx can only choose direct netmod because it does not handle any_src
    # receive when both nemod and shared memory are used.
    with_ch4_shmmods=none
fi

if test "${with_ch4_shmmods}" = "none" -o "${with_ch4_shmmods}" = "no" ; then
    AC_DEFINE(MPIDI_CH4_DIRECT_NETMOD, 1, [CH4 Directly transfers data through the chosen netmode])
else
    # This variable can be set either when CH4 controls the data transfer routine
    # or when the netmod doesn't want to implement its own locality information
    AC_DEFINE(MPIDI_BUILD_CH4_LOCALITY_INFO, 1, [CH4 should build locality info])
fi

AC_ARG_ENABLE([ch4-am-only],
              AS_HELP_STRING([--enable-ch4-am-only],[forces AM-only communication]),
              [],[enable_ch4_am_only=no])

if test "${enable_ch4_am_only}" = "yes"; then
    AC_DEFINE(MPIDI_ENABLE_AM_ONLY, 1, [Enables AM-only communication])
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

AC_ARG_WITH(ch4-max-vcis,
    [--with-ch4-max-vcis=<N>
       Select max number of VCIs to configure (default is 1; minimum is 1)],
    [], [with_ch4_max_vcis=1 ])
if test $with_ch4_max_vcis -le 0 ; then
   AC_MSG_ERROR(Number of VCIs must be greater than 0)
fi
if test $with_ch4_max_vcis -gt 1 -a $thread_granularity != MPICH_THREAD_GRANULARITY__VCI ; then
    AC_MSG_ERROR(CH4_MAX_VCIS greater than 1 requires --enable-thread-cs=per-vci)
fi
AC_DEFINE_UNQUOTED([MPIDI_CH4_MAX_VCIS], [$with_ch4_max_vcis], [Number of VCIs configured in CH4])

# Check for enable-ch4-vci-method choice
AC_ARG_ENABLE(ch4-vci-method,
	AC_HELP_STRING([--enable-ch4-vci-method=type],
			[Choose the method used for vci selection when enable-thread-cs=per-vci is selected.
                          Values may be default, zero, communicator, tag, implicit, explicit]),,enable_ch4_vci_method=default)

vci_method=MPICH_VCI__ZERO
case $enable_ch4_vci_method in
    default)
    if test "$with_ch4_max_vcis" = 1 ; then
        vci_method=MPICH_VCI__ZERO
    else
        vci_method=MPICH_VCI__COMM
    fi
    ;;
    zero)
    # Essentially single VCI
    vci_method=MPICH_VCI__ZERO
    ;;
    communicator)
    # Every communicator gets a new VCI in a round-robin fashion
    vci_method=MPICH_VCI__COMM
    ;;
    tag)
    # Explicit VCI info embedded with a tag scheme (5 bit src vci, 5 bit dst vci, plus 5 bit user)
    vci_method=MPICH_VCI__TAG
    ;;
    implicit)
    # An automatic scheme taking into account of user hints
    vci_method=MPICH_VCI__IMPLICIT
    ;;
    explicit)
    # Directly passing down vci in parameters (MPIX or Endpoint rank)
    vci_method=MPICH_VCI__EXPLICIT
    ;;
    *)
    AC_MSG_ERROR([Unrecognized value $enable_ch4_vci_method for --enable-ch4-vci-method])
    ;;
esac
AC_DEFINE_UNQUOTED([MPIDI_CH4_VCI_METHOD], $vci_method, [Method used to select vci])

AC_ARG_ENABLE(ch4-mt,
    [--enable-ch4-mt=model
       Select model for multi-threading
         direct    - Each thread directly accesses lower-level fabric (default)
         handoff   - Use the hand-off model (spawns progress thread)
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
# Currently, "handoff" and "runtime" require the followings:
# - izem linked in (--with-zm-prefix)
# - enable-thread-cs=per-vci
#
if test "$enable_ch4_mt" != "direct"; then
    if test "${with_zm_prefix}" == "no" -o "${with_zm_prefix}" == "none" -o "${enable_izem_queue}" != "yes" ; then
        AC_MSG_ERROR([Multi-threading model `${enable_ch4_mt}` requires izem queue. Set `--enable-izem={queue|all} --with-zm-prefix` and retry.])
    elif test "${enable_thread_cs}" != "per-vci" -a "${enable_thread_cs}" != "per_vci"; then
        AC_MSG_ERROR([Multi-threading model `${enable_ch4_mt}` requires `--enable-thread-cs=per-vci`.])
    fi
fi

AC_CHECK_HEADERS(sys/mman.h sys/stat.h fcntl.h)
AC_CHECK_FUNC(mmap, [], [AC_MSG_ERROR(mmap is required to build CH4)])

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
])
])dnl end AM_COND_IF(BUILD_CH4,...)

# we have to define it here to cover ch3 build
AM_CONDITIONAL([BUILD_CH4_SHM],[test "${with_ch4_shmmods}" != "none" -a "${with_ch4_shmmods}" != "no"])
AM_CONDITIONAL([BUILD_CH4_COLL_TUNING],[test -e "$srcdir/src/mpid/ch4/src/ch4_coll_globals.c"])

])dnl end _BODY

[#] end of __file__
