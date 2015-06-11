[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[llc],[build_nemesis_netmod_llc=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_LLC],[test "X$build_nemesis_netmod_llc" = "Xyes"])

])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_LLC],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:llc])

    PAC_SET_HEADER_LIB_PATH(libmemcached)
    PAC_SET_HEADER_LIB_PATH(ibverbs)
    PAC_SET_HEADER_LIB_PATH(llc)

    PAC_CHECK_HEADER_LIB(libmemcached/memcached.h,memcached,memcached,libmemcached_found=yes,libmemcached_found=no)
    if test "${libmemcached_found}" = "yes" ; then
        AC_MSG_NOTICE([libmemcached is going to be linked.])
    else
        AC_MSG_NOTICE([libmemcached was not found])
    fi

    PAC_CHECK_HEADER_LIB([infiniband/verbs.h],ibverbs,ibv_open_device,ibverbs_found=yes,ibverbs_found=no)
    if test "${ibverbs_found}" = "yes" ; then
        AC_MSG_NOTICE([libibverbs is going to be linked.])
    else
        AC_MSG_ERROR([Internal error: ibverbs was not found])
    fi

    PAC_CHECK_HEADER_LIB(llc.h,llc,LLC_init,llc_found=yes,llc_found=no)
    if test "${llc_found}" = "yes" ; then
        AC_MSG_NOTICE([libllc is going to be linked.])
    else
        AC_MSG_ERROR([Internal error: llc was not found])
    fi

    #AC_CHECK_HEADERS([stdlib.h dlfcn.h])
    #AC_CHECK_FUNCS([dlopen])
    #AC_SEARCH_LIBS([dlopen], [dl])

dnl AC_TRY_COMPILE([
dnl   #include <stdio.h>
dnl   #include <dlfcn.h>
dnl ],[
dnl   dlopen(NULL, RTLD_LAZY)
dnl ],[ac_cv_func_dlopen=yes],[ac_cv_func_dlopen=no])

    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions])
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_LLC,...)
])dnl end _BODY
[#] end of __file__
