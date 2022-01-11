/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int errors = 0;
    int cuda_support, ze_support, hip_support;
    MPI_Init(NULL, NULL);

    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &cuda_support);
    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_ZE, &ze_support);
    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_HIP, &hip_support);

    if (cuda_support != MPIX_Query_cuda_support()) {
        printf("MPIX_Query_cuda_support() return disagree with MPIX_GPU_query_support\n");
        errors++;
    }

    if (ze_support != MPIX_Query_ze_support()) {
        printf("MPIX_Query_ze_support() return disagree with MPIX_GPU_query_support\n");
        errors++;
    }

    if (hip_support != MPIX_Query_hip_support()) {
        printf("MPIX_Query_hip_support() return disagree with MPIX_GPU_query_support\n");
        errors++;
    }
#if defined(HAVE_CUDA)
    if (!cuda_support) {
        printf("Expect cuda support but not found!\n");
        errors++;
    }
#elif defined(HAVE_ZE)
    if (!ze_support) {
        printf("Expect ZE support but not found!\n");
        errors++;
    }
#elif defined(HAVE_HIP)
    if (!hip_support) {
        printf("Expect HIP support but not found!\n");
        errors++;
    }
#endif

    if (errors == 0) {
        printf("No Errors\n");
    }

    MPI_Finalize();

    return 0;
}
