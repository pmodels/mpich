[#] start of __file__
dnl
dnl _PREREQ handles the former role of mpichprereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_CH3_NEMESIS],[test "X$device_name" = "Xch3" -a "X$channel_name" = "Xnemesis"])
AM_COND_IF([BUILD_CH3_NEMESIS],[
AC_MSG_NOTICE([RUNNING PREREQ FOR ch3:nemesis])
MPID_MAX_THREAD_LEVEL=MPI_THREAD_MULTIPLE

## code that formerly lived in setup_channel.args
# Variables of interest...
#
# $with_device - device name and arguments
# $device_name - name of the device
# $device_args - contains name of channel select plus an channel args
# $channel_name - name of the channel
# $master_top_srcdir - top-level source directory
# $master_top_builddir - top-level build directory
# $ac_configure_args - all arguments passed to configure
if test -z "${channel_args}" ; then
    nemesis_networks="tcp"
else
    nemesis_networks=`echo ${channel_args} | sed -e 's/,/ /g'`
fi
export nemesis_networks

])dnl end AM_COND_IF(BUILD_CH3_NEMESIS,...)
])dnl
dnl
dnl _BODY handles the former role of configure in the subsystem
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_COND_IF([BUILD_CH3_NEMESIS],[
AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis])

## below is code that formerly lived in configure.ac

### Only include papi in CPPFLAGS configure will handle libs, and checking that it exists, etc.
##if test -n "${papi_dir}" ; then
##    PAC_APPEND_FLAG([-I${papi_dir}/include], [CPPFLAGS])
##fi

dnl AC_CHECK_HEADER(net/if.h) fails on Solaris; extra header files needed
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
],,lac_cv_header_net_if_h=yes,lac_cv_header_net_if_h=no)

echo "checking for net/if.h... $lac_cv_header_net_if_h"

if test "$lac_cv_header_net_if_h" = "yes" ; then
    AC_DEFINE(HAVE_NET_IF_H, 1, [Define if you have the <net/if.h> header file.])
fi

AC_CHECK_HEADERS([ \
    assert.h       \
    netdb.h        \
    unistd.h       \
    sched.h        \
    sys/mman.h     \
    sys/ioctl.h    \
    sys/socket.h   \
    sys/sockio.h   \
    sys/types.h    \
    errno.h        \
    sys/ipc.h      \
    sys/shm.h      \
])

# netinet/in.h often requires sys/types.h first.  With AC 2.57, check_headers
# does the right thing, which is to test whether the header is found 
# by the compiler, but this can cause problems when the header needs 
# other headers.  2.57 changes the syntax (!) of check_headers to allow 
# additional headers.
AC_CACHE_CHECK([for netinet/in.h],ac_cv_header_netinet_in_h,[
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netinet/in.h>],[int a=0;],
    ac_cv_header_netinet_in_h=yes,
    ac_cv_header_netinet_in_h=no)])
if test "$ac_cv_header_netinet_in_h" = yes ; then
    AC_DEFINE(HAVE_NETINET_IN_H,1,[Define if netinet/in.h exists])
fi

AC_ARG_ENABLE(fast, [--enable-fast - pick the appropriate options for fast execution.
This turns off error checking and timing collection],,enable_fast=no)

# make sure we support signal
AC_CHECK_HEADERS(signal.h)
AC_CHECK_FUNCS(signal)

nemesis_nets_dirs=""
nemesis_nets_strings=""
nemesis_nets_array=""   
nemesis_nets_func_decl=""       
nemesis_nets_func_array=""      
nemesis_nets_macro_defs=""
net_index=0
for net in $nemesis_networks ; do
    if test ! -d $srcdir/src/mpid/ch3/channels/nemesis/netmod/${net} ; then
        AC_MSG_ERROR([Network module ${net} is unknown "$srcdir/src/mpid/ch3/channels/nemesis/netmod/${net}"])
    fi
    net_macro=`echo $net | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    net_macro="MPIDI_NEM_${net_macro}"

    if test -z "$nemesis_nets_array" ; then
        nemesis_nets_array="$net_macro"
    else
        nemesis_nets_array="$nemesis_nets_array, $net_macro"
    fi

    if test -z "$nemesis_nets_strings" ; then
        nemesis_nets_strings="\"$net\""
    else
        nemesis_nets_strings="$nemesis_nets_strings, \"$net\""
    fi

    if test -z "$nemesis_nets_dirs" ; then
        nemesis_nets_dirs="${net}"
    else
        nemesis_nets_dirs="$nemesis_nets_dirs ${net}"
    fi

    if test -z "$nemesis_nets_func_decl" ; then
        nemesis_nets_func_decl="MPIDI_nem_${net}_funcs"
    else
        nemesis_nets_func_decl="${nemesis_nets_func_decl}, MPIDI_nem_${net}_funcs"
    fi

    if test -z "$nemesis_nets_func_array" ; then
        nemesis_nets_func_array="&MPIDI_nem_${net}_funcs"
    else
        nemesis_nets_func_array="${nemesis_nets_func_array}, &MPIDI_nem_${net}_funcs"
    fi

    if test -z "$nemesis_nets_macro_defs" ; then
        nemesis_nets_macro_defs="#define $net_macro $net_index"
    else
        nemesis_nets_macro_defs=`printf "${nemesis_nets_macro_defs}\n#define $net_macro $net_index"`
    fi

    net_index=`expr $net_index + 1`

done
nemesis_nets_array_sz=$net_index

AC_ARG_ENABLE(nemesis-dbg-nolocal, [--enable-nemesis-dbg-nolocal - enables debugging mode where shared-memory communication is disabled],
    AC_DEFINE(ENABLED_NO_LOCAL, 1, [Define to disable shared-memory communication for debugging]))

AC_ARG_ENABLE(nemesis-dbg-localoddeven, [--enable-nemesis-dbg-localoddeven - enables debugging mode where shared-memory communication is enabled only between even processes or odd processes on a node],
    AC_DEFINE(ENABLED_ODD_EVEN_CLIQUES, 1, [Define to enable debugging mode where shared-memory communication is done only between even procs or odd procs]))

AC_ARG_WITH(papi, [--with-papi[=path] - specify path where papi include and lib directories can be found],, with_papi=no)

if test "${with_papi}" != "no" ; then
    if test "${with_papi}" != "yes" ; then
        PAPI_INCLUDE="-I${with_papi}/include"
        CPPFLAGS="$CPPFLAGS $PAPI_INCLUDE"
#       LDFLAGS="$LDFLAGS -L${with_papi}/lib"
        LIBS="${with_papi}/lib/libpapi.a $LIBS"
        LIBS="${with_papi}/lib/libperfctr.a $LIBS"
    fi

    AC_CHECK_HEADER([papi.h], , [AC_MSG_ERROR(['papi.h not found in ${with_papi}/include.  Did you specify the correct path with --with-papi=?'])])

    echo -n "checking for papi libraries... "
    AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <papi.h>]],
                                   [[PAPI_library_init(PAPI_VER_CURRENT);]])], [echo "yes"], 
                                   [echo "yes" ; AC_MSG_ERROR(['Cannot link with papi:  Cannot find ${with_papi}/lib/libpapi.a or ${with_papi}/lib/libperfctr.a'])])


#    AC_CHECK_LIB(papi, PAPI_accum, , [AC_MSG_ERROR(['papi library not found.  Did you specify --with-papi=?'])])
#    AC_CHECK_LIB(perfctr, perfctr_info, , [AC_MSG_ERROR(['perfctr library not found.  Did you specify --with-papi=?'])])
fi

# handle missing mkstemp, or missing mkstemp declaration
AC_CHECK_FUNCS(mkstemp)
AC_CHECK_FUNCS(rand)
AC_CHECK_FUNCS(srand)

# Check for available shared memory functions
PAC_ARG_SHARED_MEMORY
if test "$with_shared_memory" != "mmap" -a "$with_shared_memory" != "sysv"; then
    AC_MSG_ERROR([cannot support shared memory:  need either sysv shared memory functions or mmap in order to support shared memory])
fi

AC_ARG_ENABLE(nemesis-shm-collectives, [--enable-nemesis-shm-collectives - enables use of shared memory for collective comunication within a node],
    AC_DEFINE(ENABLED_SHM_COLLECTIVES, 1, [Define to enable shared-memory collectives]))


# These are defines to turn on different optimizations.  Turn them off only for testing
AC_DEFINE(MPID_NEM_INLINE,1,[Define to turn on the inlining optimizations in Nemesis code])
AC_DEFINE(PREFETCH_CELL,1,[Define to turn on the prefetching optimization in Nemesis code])     
AC_DEFINE(USE_FASTBOX,1,[Define to use the fastboxes in Nemesis code])  

# We may need this only for tcp and related netmodules
# Check for h_addr or h_addr_list
AC_CACHE_CHECK([whether struct hostent contains h_addr_list],
pac_cv_have_haddr_list,[
AC_TRY_COMPILE([
#include <netdb.h>],[struct hostent hp;hp.h_addr_list[0]=0;],
pac_cv_have_haddr_list=yes,pac_cv_have_haddr_list=no)])
if test "$pac_cv_have_haddr_list" = "yes" ; then
    AC_DEFINE(HAVE_H_ADDR_LIST,1,[Define if struct hostent contains h_addr_list])
fi

# If we need the socket code, see if we can use struct ifconf
# sys/socket.h is needed on Solaris
AC_CACHE_CHECK([whether we can use struct ifconf],
pac_cv_have_struct_ifconf,[
AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifconf conftest; int s; s = sizeof(conftest);],
pac_cv_have_struct_ifconf=yes,pac_cv_have_struct_ifconf=no)])

# Intentionally not testing whether _SVID_SOURCE or _POSIX_C_SOURCE affects
# ifconf availability.  Making those sort of modifications at this stage
# invalidates nearly all of our previous tests, since those macros fundamentally
# change many features of the compiler on most platforms.  See ticket #1568.

if test "$pac_cv_have_struct_ifconf" = "yes" ; then
    AC_DEFINE(HAVE_STRUCT_IFCONF,1,[Define if struct ifconf can be used])
fi

AC_CACHE_CHECK([whether we can use struct ifreq],
pac_cv_have_struct_ifreq,[
AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if.h>
],[struct ifreq conftest; int s; s = sizeof(conftest);],
pac_cv_have_struct_ifreq=yes,pac_cv_have_struct_ifreq=no)])

if test "$pac_cv_have_struct_ifreq" = "yes" ; then
    AC_DEFINE(HAVE_STRUCT_IFREQ,1,[Define if struct ifreq can be used])
fi

# Check for knem options
AC_ARG_WITH(knem, [--with-knem=path - specify path where knem include directory can be found],
if test "${with_knem}" != "yes" -a "${with_knem}" != "no" ; then
    CPPFLAGS="$CPPFLAGS -I${with_knem}/include"
fi,)
AC_ARG_WITH(knem-include, [--with-knem-include=path - specify path to knem include directory],
if test "${with_knem_include}" != "yes" -a "${with_knem_include}" != "no" ; then
    CPPFLAGS="$CPPFLAGS -I${with_knem_include}"
fi,)

AC_CHECK_HEADERS([knem_io.h], pac_cv_have_knem_io_h=yes,pac_cv_have_knem_io_h=no,)
if test "${pac_cv_have_knem_io_h}" = yes ; then
    AC_DEFINE(HAVE_KNEM_IO_H,1,[Define if you have the <knem_io.h> header file.])
fi

# allow the user to select different local LMT implementations
AC_ARG_WITH(nemesis-local-lmt, [--with-nemesis-local-lmt=method - specify an implementation for local large message transfers (LMT).  Method is one of: 'default', 'shm_copy', 'knem', or 'none'.  'default' is the same as 'shm_copy'.],,with_nemesis_local_lmt=default)
case "$with_nemesis_local_lmt" in
    shm_copy|default)
    local_lmt_impl=MPID_NEM_LOCAL_LMT_SHM_COPY
    ;;
    dma|shm_dma|knem)
    if test "${pac_cv_have_knem_io_h}" != yes ; then
        AC_MSG_ERROR([Failed to find knem_io.h for nemesis-local-lmt=knem])
    fi
    local_lmt_impl=MPID_NEM_LOCAL_LMT_DMA
    ;;
    vmsplice)
    local_lmt_impl=MPID_NEM_LOCAL_LMT_VMSPLICE
    ;;
    none)
    local_lmt_impl=MPID_NEM_LOCAL_LMT_NONE
    ;;
    *)
    AC_MSG_ERROR([Unrecognized value $with_nemesis_local_lmt for --with-nemesis-local-lmt])
    ;;
esac
AC_DEFINE_UNQUOTED([MPID_NEM_LOCAL_LMT_IMPL],$local_lmt_impl,[Method for local large message transfers.])

AC_ARG_ENABLE(nemesis-lock-free-queues,
              [--enable-nemesis-lock-free-queues - Use atomic instructions and lock-free queues for shared memory communication.  Lock-based queues will be used otherwise.  The default is enabled (lock-free).],
              , [enable_nemesis_lock_free_queues=yes])
if test "$enable_nemesis_lock_free_queues" = "yes" ; then
    AC_DEFINE(MPID_NEM_USE_LOCK_FREE_QUEUES, 1, [Define to enable lock-free communication queues])
fi


AC_SUBST(device_name)
AC_SUBST(channel_name)
AC_SUBST(nemesis_networks)
AC_SUBST(nemesis_nets_dirs)
AC_SUBST(nemesis_nets_strings)
AC_SUBST(nemesis_nets_func_decl)
AC_SUBST(nemesis_nets_func_array)
AC_SUBST(nemesis_nets_array)
AC_SUBST(nemesis_nets_array_sz)
AC_SUBST(nemesis_nets_macro_defs)

AC_SUBST(mmx_copy_s)
AC_SUBST(PAPI_INCLUDE)
AC_SUBST(AS, [as])

AC_CONFIG_FILES([
src/mpid/ch3/channels/nemesis/include/mpid_nem_net_module_defs.h
src/mpid/ch3/channels/nemesis/src/mpid_nem_net_array.c
])

])dnl end AM_COND_IF(BUILD_CH3_NEMESIS,...)
])dnl end _BODY
[#] end of __file__
