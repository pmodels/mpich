[#] start of __file__
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/sched
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/datatype
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/thread
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/bc
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/genq
dnl MPICH_SUBCFG_BEFORE=src/mpid/common/stream_workq

dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
[#] expansion is: PAC_SUBCFG_PREREQ_[]PAC_SUBCFG_AUTO_SUFFIX
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH4],[test "$device_name" = "ch4"])

AM_COND_IF([BUILD_CH4],[
AC_MSG_NOTICE([RUNNING PREREQ FOR CH4 DEVICE])

# check availability of libfabric, ucx (for purpose of setting default)
m4_define([libfabric_embedded_dir],[modules/libfabric])
m4_define([ucx_embedded_dir],[modules/ucx])
PAC_PROBE_HEADER_LIB(libfabric,[rdma/fabric.h], [fabric], [fi_getinfo])
PAC_PROBE_HEADER_LIB(ucx,[ucp/api/ucp.h], [ucp], [ucp_config_read], [-lucs -lucm -luct])

# the CH4 device depends on the common NBC scheduler code
build_mpid_common_sched=yes
build_mpid_common_datatype=yes
build_mpid_common_thread=yes
build_mpid_common_bc=yes
build_mpid_common_genq=yes
build_mpid_common_stream_workq=yes

MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE
MPID_MAX_PROCESSOR_NAME=128
MPID_MAX_ERROR_STRING=512

pac_ch4_choice=none

# $device_args - contains the netmods
if test -z "${device_args}" ; then
  AS_CASE([$host_os],
  [linux*],[
    dnl attempt to choose a netmod from the installed libraries
    if test $pac_have_ucx = "yes" -a $pac_have_libfabric = "no" ; then
        pac_ch4_choice="ucx-have-ucx"
        ch4_netmods=ucx
    elif test $pac_have_ucx = "no" -a $pac_have_libfabric = "yes" ; then
        pac_ch4_choice="ofi-have-libfabric"
        ch4_netmods=ofi
    else
        pac_ch4_choice="ofi-default"
        ch4_netmods=ofi
    fi],
    [
      dnl non-linux use libfabric
      pac_ch4_choice="ofi-default"
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
    if test -z "$ch4_netmod_part_decl" ; then
        ch4_netmod_part_decl="MPIDI_${net_upper}_part_t ${net};"
    else
        ch4_netmod_part_decl="${ch4_netmod_part_decl} \\
MPIDI_${net_upper}_part_t ${net};"
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
AC_SUBST(ch4_netmod_part_decl)
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
AM_SUBST_NOTMAKE(ch4_netmod_part_decl)

AC_ARG_ENABLE(ch4-netmod-inline, [
  --enable-ch4-netmod-inline  Enables inlined netmod build when a single netmod is used
                              level:
                                yes       - Enabled (default)
                                no        - Disabled (may improve build times and code size)
],,enable_ch4_netmod_inline=yes)


AC_ARG_ENABLE(ch4-netmod-direct, [
  --enable-ch4-netmod-direct (Deprecated in favor of ch4-netmod-inline)
                             Enables inlined netmod build when a single netmod is used
                             level:
                               yes       - Enabled (default)
                               no        - Disabled (may improve build times and code size)
],,)

if test "$ch4_nets_array_sz" = "1" && (test "$enable_ch4_netmod_inline" = "yes" || test "$enable_ch4_netmod_direct" = "yes") ;  then
   PAC_APPEND_FLAG([-DNETMOD_INLINE=__netmod_inline_${ch4_netmods}__], [CPPFLAGS])
fi

AC_ARG_ENABLE([ch4-direct],
              [  --enable-ch4-direct   DO NOT USE!  Use --without-ch4-shmmods instead],
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

dnl Note: the maximum of 64 is due to the fact that we use 6 bits in the
dnl request handle to encode pool index
AC_ARG_WITH(ch4-max-vcis, [
  --with-ch4-max-vcis=<N> - Select max number of VCIs to configure (default
                            is 64; minimum is 1; maximum is 64)],
    [], [with_ch4_max_vcis=64])

if test $with_ch4_max_vcis -lt 1 -o $with_ch4_max_vcis -gt 64; then
   AC_MSG_ERROR(Number of VCIs must be between 1 and 64)
fi
AC_DEFINE_UNQUOTED([MPIDI_CH4_MAX_VCIS], [$with_ch4_max_vcis], [Number of VCIs configured in CH4])

# Check for enable-ch4-vci-method choice
AC_ARG_ENABLE(ch4-vci-method,
	AS_HELP_STRING([--enable-ch4-vci-method=type],
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

AC_ARG_ENABLE(ch4-mt, [
  --enable-ch4-mt=model - Select model for multi-threading
                            direct    - Each thread directly accesses lower-level fabric (default)
                            lockless  - Use the thread safe serialization model supported by the provider
                            runtime   - Determine the model at runtime through a CVAR
],,enable_ch4_mt=direct)

case $enable_ch4_mt in
     direct)
         AC_DEFINE([MPIDI_CH4_USE_MT_DIRECT], [1],
            [Define to enable direct multi-threading model])
        ;;
     lockless)
         AC_DEFINE([MPIDI_CH4_USE_MT_LOCKLESS], [1],
            [Define to enable lockless multi-threading model])
        ;;
     runtime)
         AC_DEFINE([MPIDI_CH4_USE_MT_RUNTIME], [1],
            [Define to enable runtime multi-threading model])
        ;;
     *)
        AC_MSG_ERROR([Multi-threading model ${enable_ch4_mt} is unknown])
        ;;
esac

AC_CHECK_HEADERS(sys/mman.h sys/stat.h fcntl.h)
AC_CHECK_FUNC(mmap, [], [AC_MSG_ERROR(mmap is required to build CH4)])

AC_CHECK_FUNCS(gethostname)
if test "$ac_cv_func_gethostname" = "yes" ; then
    # Do we need to declare gethostname?
    PAC_FUNC_NEEDS_DECL([#include <unistd.h>],gethostname)
fi

AC_CONFIG_FILES([
src/mpid/ch4/src/mpid_ch4_net_array.c
src/mpid/ch4/include/netmodpre.h
])
])dnl end AM_COND_IF(BUILD_CH4,...)

# we have to define it here to cover ch3 build
AM_CONDITIONAL([BUILD_CH4_SHM],[test "${with_ch4_shmmods}" != "none" -a "${with_ch4_shmmods}" != "no"])
AM_CONDITIONAL([BUILD_CH4_COLL_TUNING],[test -e "$srcdir/src/mpid/ch4/src/ch4_coll_globals.c"])

])dnl end _BODY

dnl Summary notes at the end of configure
AC_DEFUN([PAC_CH4_CONFIG_SUMMARY], [
    t_netmod=$ch4_netmods
    if test "$ch4_netmods" = "ofi" -a "$with_libfabric" = "embedded"; then
        t_netmod="ofi (embedded libfabric)"
    elif test "$ch4_netmods" = "ucx" -a "$with_ucx" = "embedded"; then
        t_netmod="ucx (embedded)"
    fi
    t_xpmem=""
    if test "$pac_have_xpmem" = "yes" ; then
        t_xpmem="xpmem"
    fi
    t_gpu="disabled"
    if test -n "${GPU_SUPPORT}" ; then
        t_gpu="${GPU_SUPPORT}"
    fi
    cat <<EOF
***
*** device      : ch4:${t_netmod}
*** shm feature : ${ch4_shm} $t_xpmem
*** gpu support : ${t_gpu}
***
EOF    

    if test "$pac_ch4_choice" = "ofi-default" ; then
        cat <<EOF
  MPICH is configured with device ch4:ofi, which should work
  for TCP networks and any high-bandwidth interconnect
  supported by libfabric. MPICH can also be configured with
  "--with-device=ch4:ucx", which should work for TCP networks
  and any high-bandwidth interconnect supported by the UCX
  library. In addition, the legacy device ch3 (--with-device=ch3)
  is also available. 
EOF        
    elif test "$pac_ch4_choice" = "ofi-have-libfabric" ; then
        cat <<EOF
  MPICH is configured with device ch4:ofi using libfabric.
  Alternatively, MPICH can be configured using
  "--with-device=ch4:ucx" to use the UCX library. In addition,
  the legacy device ch3 (--with-device=ch3) is also available.
EOF        
    elif test "$pac_ch4_choice" = "ofi-have-ucx" ; then
        cat <<EOF
  MPICH is configured with device ch4:ucx using UCX library.
  Alternatively, MPICH can be configured using
  "--with-device=ch4:ofi" to use the libfabric library. In
  addition, the legacy device ch3 (--with-device=ch3) is also
  available.
EOF        
    fi
])

[#] end of __file__
