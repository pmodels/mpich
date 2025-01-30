##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##


##########################################################################
##### capture user arguments
##########################################################################

# --with-hip-sm
AC_ARG_WITH([hip-sm],
            [
  --with-hip-sm=<options>
          Comma-separated list of below options:
                auto - let HIPCC to auto detect the target_id
                <numeric> - specific target_id to use, see https://llvm.org/docs/AMDGPUUsage.html
            ],,
            [with_hip_sm=auto])


# --with-hip
PAC_SET_HEADER_LIB_PATH([hip])
if test "$with_hip" != "no" ; then
    PAC_PUSH_FLAG(CPPFLAGS)
    PAC_APPEND_FLAG([-D__HIP_PLATFORM_AMD__], [CPPFLAGS])
    PAC_CHECK_HEADER_LIB([hip/hip_runtime_api.h],[amdhip64],[hipStreamSynchronize],[have_hip=yes],[have_hip=no])
    AC_CHECK_MEMBER([struct hipPointerAttribute_t.memoryType],
                    [AC_DEFINE(HIP_USE_MEMORYTYPE, 1, [Define if struct hipPointerAttribute_t use memoryType (pre-v6.0)])],
                    [],
                    [[#include <hip/hip_runtime_api.h>]])
    PAC_POP_FLAG(CPPFLAGS)
    if test "${have_hip}" = "yes" ; then
        AC_MSG_CHECKING([whether hipcc works])
        cat>conftest.c<<EOF
        #include <hip/hip_runtime_api.h>
        void foo(int deviceId) {
        hipError_t ret;
        ret = hipGetDevice(&deviceId);
        }
EOF
        PAC_PUSH_FLAG(CPPFLAGS)
        PAC_APPEND_FLAG([-D__HIP_PLATFORM_AMD__], [CPPFLAGS])
        ${with_hip}/bin/hipcc -c conftest.c 2> /dev/null
        PAC_POP_FLAG(CPPFLAGS)
        if test "$?" = "0" ; then
            AC_DEFINE([HAVE_HIP],[1],[Define is HIP is available])
            PAC_APPEND_FLAG([-D__HIP_PLATFORM_AMD__], [CPPFLAGS])
            AS_IF([test -n "${with_hip}"],[HIPCC=${with_hip}/bin/hipcc],[HIPCC=hipcc])
            AC_SUBST(HIPCC)
            # hipcc compiled applications need libstdc++ to be able to link
            # with a C compiler
            PAC_PUSH_FLAG([LIBS])
            PAC_APPEND_FLAG([-lstdc++],[LIBS])
            AC_LINK_IFELSE(
                [AC_LANG_PROGRAM([int x = 5;],[x++;])],
                [libstdcpp_works=yes],
                [libstdcpp_works=no])
            PAC_POP_FLAG([LIBS])
            if test "${libstdcpp_works}" = "yes" ; then
                PAC_APPEND_FLAG([-lstdc++],[LIBS])
                AC_MSG_RESULT([yes])
            else
                have_hip=no
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([hipcc compiled applications need libstdc++ to be able to link with a C compiler])
            fi
        else
            have_hip=no
            AC_MSG_RESULT([no])
        fi
        rm -f conftest.*
    fi
fi
AM_CONDITIONAL([BUILD_HIP_BACKEND], [test x${have_hip} = xyes])

# --with-hip-p2p
AC_ARG_ENABLE([hip-p2p],AS_HELP_STRING([--enable-hip-p2p={yes|no|cliques}],[controls HIP P2P capability]),,
              [enable_hip_p2p=yes])
if test "${enable_hip_p2p}" = "yes" ; then
    AC_DEFINE([HIP_P2P],[HIP_P2P_ENABLED],[Define if HIP P2P is enabled])
elif test "${enable_hip_p2p}" = "cliques" ; then
    AC_DEFINE([HIP_P2P],[HIP_P2P_CLIQUES],[Define if HIP P2P is enabled in clique mode])
else
    AC_DEFINE([HIP_P2P],[HIP_P2P_DISABLED],[Define if HIP P2P is disabled])
fi


##########################################################################
##### analyze the user arguments and setup internal infrastructure
##########################################################################

if test "${have_hip}" = "yes" ; then
    for maj_version in 4 3 2 1; do
        version=$((maj_version * 1000))
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                              #include <hip/hip_runtime.h>
                              int x[[HIP_VERSION - $version]];
                          ],)],[hip_version=${maj_version}],[])
        if test ! -z ${hip_version} ; then break ; fi
    done
    HIP_GENCODE=
    if test "${with_hip_sm}" != "auto"; then
        PAC_PUSH_FLAG([IFS])
        IFS=","
        HIP_SM=
        for sm in ${with_hip_sm} ; do
            PAC_APPEND_FLAG([$sm], [HIP_SM])
        done
        PAC_POP_FLAG([IFS])

        for sm in ${HIP_SM} ; do
            if test -z "${HIP_GENCODE}" ; then
                HIP_GENCODE="--offload-arch=${sm}"
            else
                HIP_GENCODE="${HIP_GENCODE} --offload-arch=${sm}"
            fi
        done
    fi
    AC_SUBST(HIP_GENCODE)

    supported_backends="${supported_backends},hip"
    backend_info="${backend_info}
HIP backend specific options:
      HIP GENCODE: ${with_hip_sm} (${HIP_GENCODE})"
fi
