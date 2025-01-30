##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##


##########################################################################
##### capture user arguments
##########################################################################

# --with-cuda-sm
AC_ARG_WITH([cuda-sm],
            [
  --with-cuda-sm=<options> (https://arnon.dk/matching-sm-architectures-arch-and-gencode-for-various-nvidia-cards/)
          Comma-separated list of below options:
                auto - automatically build compatibility for all GPUs visible, any other specified compatibilities are ignored
                all-major - build compatibility for all major GPU versions (sm_*0) supported by the CUDA version

                # Kepler architecture
                kepler - build compatibility for all Kepler GPUs
                30     - generic kepler architecture (generic - Tesla K40/K80, GeForce 700, GT-730)
                35     - specific Tesla K40 (adds support for dynamic parallelism)
                37     - specific Tesla K80 (adds more registers)

                # Maxwell architecture
                maxwell - build compatibility for all Maxwell GPUs
                50      - Tesla/Quadro M series
                52      - Quadro M6000, GeForce 900, GTX-970, GTX-980, GTX Titan X
                53      - Tegra (Jetson) TX1 / Tegra X1, Drive CX, Drive PX, Jetson Nano

                # Pascal architecture
                pascal - build compatibility for all Pascal GPUs
                60     - Quadro GP100, Tesla P100, DGX-1 (Generic Pascal)
                61     - GTX 1080, GTX 1070, GTX 1060, GTX 1050, GTX 1030, Titan Xp,
                         Tesla P40, Tesla P4, Discrete GPU on the NVIDIA Drive PX2
                62     - Integrated GPU on the NVIDIA Drive PX2, Tegra (Jetson) TX2

                # Volta architecture
                volta - build compatibility for all Volta GPUs
                70    - DGX-1 with Volta, Tesla V100, GTX 1180 (GV104), Titan V, Quadro GV100
                72    - Jetson AGX Xavier, Drive AGX Pegasus, Xavier NX

                # Turing architecture
                turing - build compatibility for all Turing GPUs
                75     - GTX/RTX Turing - GTX 1660 Ti, RTX 2060, RTX 2070, RTX 2080, Titan RTX,
                         Quadro RTX 4000, Quadro RTX 5000, Quadro RTX 6000, Quadro RTX 8000,
                         Quadro T1000/T2000, Tesla T4

                # Ampere architecture
                ampere - build compatibility for all Ampere GPUs
                80     - A100, A30
                86     - RTX Ampere, MX570, A40, A16, A10, A2
                87     - Jetson AGX Orin and Drive AGX Orin

                # Ada architecture
                ada    - build compatibility for all Ada GPUs
                89     - GeForce RTX 4090, RTX 4080, RTX 6000, Tesla L40

                # Hopper architecture
                hopper - build compatibility for all Hopper GPUs
                90     - NVIDIA H100 (GH100)
                90a    - add acceleration for features like wgmma and setmaxnreg. Required for NVIDIA CUTLASS

                # Other
                <numeric> - specific SM numeric to use
            ],,
            [with_cuda_sm=auto])


# --with-cuda
AC_ARG_VAR([NVCC], [nvcc compiler to use])
PAC_SET_HEADER_LIB_PATH([cuda])
if test "$with_cuda" != "no" ; then
    PAC_CHECK_HEADER_LIB([cuda_runtime_api.h],[cudart],[cudaStreamSynchronize],[have_cuda=yes],[have_cuda=no])
    if test "${have_cuda}" = "yes" ; then
        # check if NVCC has been set by the user
        if test -z "$NVCC" ; then
            if test -n "$ac_save_CC" ; then
                NVCC_FLAGS="$NVCC_FLAGS -ccbin $ac_save_CC"
                # - pgcc/nvc doesn't work, use pgc++/nvc++ instead
                # - Extra options such as `gcc -std=gnu99` doesn't work, strip the option
                NVCC_FLAGS=$(echo $NVCC_FLAGS | sed -e 's/nvc/nvc++/g' -e 's/pgcc/pgc++/g' -e's/ -std=.*//g')
            fi

            # try nvcc from PATH if 'with-cuda' does not contain a valid path
            if test -d ${with_cuda} ; then
                nvcc_bin=${with_cuda}/bin/nvcc
            else
                nvcc_bin=nvcc
            fi

            # append the flags to be passed to nvcc
            NVCC="$nvcc_bin $NVCC_FLAGS"
        else
            AC_MSG_WARN([Using user-provided nvcc: '${NVCC}'])
        fi

        AC_SUBST(NVCC)

        # save language settings, customize ac_ext and ac_compile to support CUDA
        AC_LANG_PUSH([C])
        ac_ext=cu
        ac_compile="$NVCC -c conftest.$ac_ext >&5"
        AC_MSG_CHECKING([whether nvcc works])
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([__global__ void foo(int x) {}],[])],
        [
            AC_DEFINE([HAVE_CUDA],[1],[Define is CUDA is available])
            AC_MSG_RESULT([yes])
        ],[
            have_cuda=no
            AC_MSG_RESULT([no])
        ])

        # done with CUDA, back to C
        AC_LANG_POP([C])

        if test "${have_cuda}" != "no" ; then
            # nvcc compiled applications need libstdc++ to be able to link
            # with a C compiler
            AC_MSG_CHECKING([if $CC can link libstdc++])
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
                have_cuda=no
                AC_MSG_RESULT([no])
            fi
        fi
    fi

    # if the user requested CUDA and it didn't work, throw an error
    if test "${have_cuda}" = "no" -a "$with_cuda" != ""; then
        AC_MSG_ERROR([CUDA was requested but it is not functional])
    fi
fi
AM_CONDITIONAL([BUILD_CUDA_BACKEND], [test x${have_cuda} = xyes])
AM_CONDITIONAL([BUILD_CUDA_TESTS], [test x${have_cuda} = xyes])


# --with-cuda-p2p
AC_ARG_ENABLE([cuda-p2p],AS_HELP_STRING([--enable-cuda-p2p={yes|no|cliques}],[controls CUDA P2P capability]),,
              [enable_cuda_p2p=yes])
if test "${enable_cuda_p2p}" = "yes" ; then
    AC_DEFINE([CUDA_P2P],[CUDA_P2P_ENABLED],[Define if CUDA P2P is enabled])
elif test "${enable_cuda_p2p}" = "cliques" ; then
    AC_DEFINE([CUDA_P2P],[CUDA_P2P_CLIQUES],[Define if CUDA P2P is enabled in clique mode])
else
    AC_DEFINE([CUDA_P2P],[CUDA_P2P_DISABLED],[Define if CUDA P2P is disabled])
fi


##########################################################################
##### analyze the user arguments and setup internal infrastructure
##########################################################################

if test "${have_cuda}" = "yes" ; then
    for version in 12000 11080 11050 11010 11000 10000 9000 8000 7000 6000 5000 ; do
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                              #include <cuda.h>
                              int x[[CUDA_VERSION - $version]];
                          ],)],[cuda_version=${version}],[])
        if test ! -z ${cuda_version} ; then break ; fi
    done

    CUDA_SM=
    case "$with_cuda_sm" in
        *auto*)
            dnl process auto detection
            PAC_PUSH_FLAG([IFS])
            IFS=" "
            AC_MSG_CHECKING([for CUDA compute capability auto detection])
            AC_LANG_PUSH([C])
            AC_RUN_IFELSE(
                [AC_LANG_PROGRAM(
                  [
                      #include <cuda_runtime.h>
                      #include <stdio.h>
                  ],
                  [
                      int count = 0;
                      if (cudaSuccess != cudaGetDeviceCount(&count)) return -1;
                      if (count == 0) return -1;
                      for (int device = 0; device < count; ++device)
                      {
                          struct cudaDeviceProp prop;
                          if (cudaSuccess == cudaGetDeviceProperties(&prop, device))
                          printf("%d.%d ", prop.major, prop.minor);
                      }
                      return 0;
                  ]
                )],
                [
                    cuda_output=$(./conftest$EXEEXT | xargs -n1 | sort -u | xargs)
                    for sm in $cuda_output; do
                        sm_no_decimal=`echo $sm | tr -d '.'`
                        PAC_APPEND_FLAG([$sm_no_decimal],[CUDA_SM])
                    done
                    with_cuda_sm=
                    AC_MSG_RESULT([yes])
                ],
                [
                    with_cuda_sm=all-major
                    AC_MSG_RESULT([no])
                ]
              )
            AC_LANG_POP([C])
            PAC_POP_FLAG([IFS])
            ;;
        *)
            ;;
    esac

    PAC_PUSH_FLAG([IFS])
    IFS=","
    for sm in ${with_cuda_sm} ; do
        case "$sm" in
            all-major)
                if test ${cuda_version} -ge 11080 ; then
                    # maxwell (52) to hopper (90)
                    supported_cuda_sms="52 60 70 80 90"
                elif test ${cuda_version} -ge 11010 ; then
                    # maxwell (52) to ampere (80)
                    supported_cuda_sms="52 60 70 80"
                elif test ${cuda_version} -ge 11000 ; then
                    # maxwell (52) to ampere (80)
                    supported_cuda_sms="52 60 70 80"
                elif test ${cuda_version} -ge 10000 ; then
                    # kepler (30) to volta (70)
                    supported_cuda_sms="30 50 60 70"
                elif test ${cuda_version} -ge 9000 ; then
                    # kepler (30) to volta (70)
                    supported_cuda_sms="30 50 60 70"
                elif test ${cuda_version} -ge 8000 ; then
                    # kepler (30) to pascal (60)
                    supported_cuda_sms="30 50 60"
                elif test ${cuda_version} -ge 6000 ; then
                    # kepler (30) to maxwell (50)
                    supported_cuda_sms="30 50"
                elif test ${cuda_version} -ge 5000 ; then
                    # kepler (30)
                    supported_cuda_sms="30"
                fi

                for supported_cuda_sm in $supported_cuda_sms ; do
                    PAC_APPEND_FLAG([$supported_cuda_sm],[CUDA_SM])
                done
                ;;

            kepler)
                PAC_APPEND_FLAG([30],[CUDA_SM])
                PAC_APPEND_FLAG([35],[CUDA_SM])
                PAC_APPEND_FLAG([37],[CUDA_SM])
                ;;

            maxwell)
                PAC_APPEND_FLAG([50],[CUDA_SM])
                PAC_APPEND_FLAG([52],[CUDA_SM])
                PAC_APPEND_FLAG([53],[CUDA_SM])
                ;;

            pascal)
                PAC_APPEND_FLAG([60],[CUDA_SM])
                PAC_APPEND_FLAG([61],[CUDA_SM])
                PAC_APPEND_FLAG([62],[CUDA_SM])
                ;;

            volta)
                PAC_APPEND_FLAG([70],[CUDA_SM])
                PAC_APPEND_FLAG([72],[CUDA_SM])
                ;;

            turing)
                PAC_APPEND_FLAG([75],[CUDA_SM])
                ;;

            ampere)
                PAC_APPEND_FLAG([80],[CUDA_SM])
                PAC_APPEND_FLAG([86],[CUDA_SM])
                ;;

            ada)
                PAC_APPEND_FLAG([89],[CUDA_SM])
                ;;

            hopper)
                PAC_APPEND_FLAG([90],[CUDA_SM])
                PAC_APPEND_FLAG([90a],[CUDA_SM])
                ;;

            none)
                ;;

            *)
                PAC_APPEND_FLAG([$sm],[CUDA_SM])
                ;;
          esac
    done
    PAC_POP_FLAG([IFS])

    for sm in ${CUDA_SM} ; do
        if test -z "${CUDA_GENCODE}" ; then
            CUDA_GENCODE="-gencode=arch=compute_${sm},code=sm_${sm}"
        else
            CUDA_GENCODE="${CUDA_GENCODE} -gencode=arch=compute_${sm},code=sm_${sm}"
        fi
    done
    AC_SUBST(CUDA_GENCODE)

    if test -z "${CUDA_GENCODE}" ; then
        AC_MSG_ERROR([--with-cuda-sm not specified; either specify it or disable cuda support])
    fi

    supported_backends="${supported_backends},cuda"
    backend_info="${backend_info}
CUDA backend specific options:
      CUDA GENCODE: ${with_cuda_sm} (${CUDA_GENCODE})"
fi
