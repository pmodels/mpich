/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int cuda_support, ze_support;
    MPI_Init(NULL, NULL);

    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_CUDA, &cuda_support);
    MPIX_GPU_query_support(MPIX_GPU_SUPPORT_ZE, &ze_support);

    if (cuda_support && ze_support) {
        printf("CUDA and ZE are supported\n");
    } else if (cuda_support) {
        printf("CUDA is supported\n");
    } else if (ze_support) {
        printf("ZE is supported\n");
    } else {
        printf("No GPUs are supported\n");
    }

    MPI_Finalize();

    return 0;
}
