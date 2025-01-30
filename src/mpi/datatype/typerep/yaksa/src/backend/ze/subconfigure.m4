##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##


##########################################################################
##### capture user arguments
##########################################################################

# --with-ze
PAC_SET_HEADER_LIB_PATH([ze])
if test "$with_ze" != "no" ; then
    PAC_CHECK_HEADER_LIB([level_zero/ze_api.h],[ze_loader],[zeCommandQueueCreate],[have_ze=yes],[have_ze=no])
    AC_MSG_CHECKING([whether ocloc is installed])
    if ! command -v ocloc &> /dev/null; then
        if test "$with_ze" = "yes" ; then
            AC_MSG_ERROR([ocloc not found; either install it or disable ze support])
        else
            AC_MSG_RESULT([no])
            have_ze=no
        fi
    else
        AC_MSG_RESULT([yes])
    fi
    # ze_api.h relies on support for c11
    if test "${have_ze}" = "yes" ; then
        PAC_PUSH_FLAG([CFLAGS])
        CFLAGS="$CFLAGS -Werror"
        AC_CACHE_CHECK([for -Werror],ac_cv_werror,[
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([],[])],ac_cv_werror=yes,ac_cv_werror=no)])
        if test "${ac_cv_werror}" = "yes" ; then
            AC_CACHE_CHECK([for c11 support],ac_cv_support_c11,[
            AC_COMPILE_IFELSE([AC_LANG_SOURCE([
    typedef struct _ze_ipc_mem_handle_t
    {
        int data;
    } ze_ipc_mem_handle_t;
    typedef struct _ze_ipc_mem_handle_t ze_ipc_mem_handle_t;
            ],[])],ac_cv_support_c11=yes,ac_cv_support_c11=no)])
            if test "${ac_cv_support_c11}" = "no" ; then
                have_ze=no
            fi
        fi
        PAC_POP_FLAG([CFLAGS])
    fi
    if test "${have_ze}" = "yes" ; then
        AC_DEFINE([HAVE_ZE],[1],[Define is ZE is available])
    elif test "$with_ze" != ""; then
        AC_MSG_ERROR([ZE was requested but it is not functional])
    fi
fi
AM_CONDITIONAL([BUILD_ZE_BACKEND], [test x${have_ze} = xyes])
AM_CONDITIONAL([BUILD_ZE_TESTS], [test x${have_ze} = xyes])


# --with-ze-p2p
AC_ARG_ENABLE([ze-p2p],AS_HELP_STRING([--enable-ze-p2p={yes|no|cliques}],[controls ZE P2P capability]),,
              [enable_ze_p2p=yes])
if test "${have_ze}" = "yes" ; then
    if test "${enable_ze_p2p}" = "yes" ; then
        AC_DEFINE([ZE_P2P],[ZE_P2P_ENABLED],[Define if ZE P2P is enabled])
    elif test "${enable_ze_p2p}" = "cliques" ; then
        AC_DEFINE([ZE_P2P],[ZE_P2P_CLIQUES],[Define if ZE P2P is enabled in clique mode])
    else
        AC_DEFINE([ZE_P2P],[ZE_P2P_DISABLED],[Define if ZE P2P is disabled])
    fi
fi

extra_ocloc_options=

# --ze-revision-id=[integer]
# PVC native compilation requires revision_id
AC_ARG_ENABLE([ze-revision-id],AS_HELP_STRING([--enable-ze-revision-id=int],[specify revision id]),,
              [enable_ze_revision_id=-1])
if test $enable_ze_revision_id != -1; then
    extra_ocloc_options="-revision_id $enable_ze_revision_id"
fi

# --ze-native=[skl|dg1|ats|pvc]
enable_multi_native=no
AC_ARG_ENABLE([ze-native],AS_HELP_STRING([--enable-ze-native=device],[compile GPU kernel to native binary]),,
              [enable_ze_native=no])
if test "${have_ze}" = "yes" -a x"${enable_ze_native}" != xno; then
    AC_MSG_CHECKING([whether ocloc works with target device])
    cat>conftest.cl<<EOF
    __kernel void foo(int x) {}
EOF
    rm -f conftest.ar
    ocloc compile -file conftest.cl -device ${enable_ze_native} ${extra_ocloc_options} -options "-cl-std=CL2.0" > /dev/null 2>&1
    if test "$?" = "0" ; then
        AC_MSG_RESULT([yes])
        if test -f conftest.ar; then
            enable_multi_native=yes
        fi
    else
        AC_MSG_RESULT([no])
        enable_ze_native=no
        AC_MSG_ERROR([ocloc compiler is not compatible with ze_native])
    fi
    rm -f conftest.*
fi
if test x"${enable_ze_native}" != xno; then
    AC_DEFINE(ZE_NATIVE, 1, [Compile kernels to binary])
else
    AC_DEFINE(ZE_NATIVE, 0, [No native format])
fi
AC_SUBST(enable_ze_native)
AC_SUBST(extra_ocloc_options)
AM_CONDITIONAL([BUILD_ZE_NATIVE],[test x"$enable_ze_native" != xno -a x"$enable_multi_native" != xyes])
AM_CONDITIONAL([BUILD_ZE_NATIVE_MULTIPLE],[test x"$enable_multi_native" != xno])


##########################################################################
##### analyze the user arguments and setup internal infrastructure
##########################################################################

if test "${have_ze}" = "yes" ; then
    supported_backends="${supported_backends},ze"
    if test x${enable_ze_native} != xno; then
        supported_backends="$supported_backends($enable_ze_native)"
    fi
fi
