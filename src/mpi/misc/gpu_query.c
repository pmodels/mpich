/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_GPU_query_support_impl(int gpu_type, int *is_supported)
{
    int mpi_errno = MPI_SUCCESS;

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

        case MPIX_GPU_SUPPORT_HIP:
            if (type == MPL_GPU_TYPE_HIP)
                *is_supported = 1;
            break;

        default:
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**badgputype");
        }
    }

fn_fail:
    return mpi_errno;
}
