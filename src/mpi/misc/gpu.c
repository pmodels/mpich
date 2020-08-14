/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Aint_add */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_GPU_query_support = PMPIX_GPU_query_support
#pragma weak MPIX_Query_cuda_support = PMPIX_Query_cuda_support
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_GPU_query_support  MPIX_GPU_query_support
#pragma _HP_SECONDARY_DEF PMPIX_Query_cuda_support MPIX_Query_cuda_support
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_GPU_query_support as PMPIX_GPU_query_support
#pragma _CRI duplicate MPIX_Query_cuda_support as PMPIX_Query_cuda_support
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_GPU_query_support(int gpu_type, int *is_supported)
    __attribute__ ((weak, alias("PMPIX_GPU_query_support")));
int MPIX_Query_cuda_support(void)
    __attribute__ ((weak, alias("PMPIX_Query_cuda_support")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_GPU_query_support
#define MPIX_GPU_query_support PMPIX_GPU_query_support
#undef MPIX_Query_cuda_support
#define MPIX_Query_cuda_support PMPIX_Query_cuda_support
#endif


/*@
MPIX_GPU_query_support - Returns the type of GPU supported

Input Parameters:
+ gpu_type - the GPU type being queried for (integer)
- is_supported - whether supported or not (integer)

Notes:
Query for the various GPU types whether they are supported by MPICH or
not.
@*/

int MPIX_GPU_query_support(int gpu_type, int *is_supported)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_GPU_QUERY_SUPPORT);
    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIX_GPU_QUERY_SUPPORT);

    *is_supported = 0;
    if (MPIR_CVAR_ENABLE_GPU) {
        MPL_gpu_type_t type;
        MPL_gpu_query_support(&type);

        switch (gpu_type) {
            case MPIX_GPU_SUPPORT_CUDA:
                if (type == MPL_GPU_TYPE_CUDA)
                    *is_supported = 1;
                break;

            case MPIX_GPU_SUPPORT_ZE:
                if (type == MPL_GPU_TYPE_ZE)
                    *is_supported = 1;
                break;

            default:
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**badgputype");
        }
    }

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIX_GPU_QUERY_SUPPORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIX_Query_cuda_support(void)
{
    int is_supported = 0;
    int mpi_errno;

    mpi_errno = MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &is_supported);
    assert(mpi_errno);

    return is_supported;
}
