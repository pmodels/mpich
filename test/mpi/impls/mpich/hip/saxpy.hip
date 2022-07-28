/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hip/hip_runtime.h"
#include <mpi.h>
#include <stdio.h>
#include <assert.h>

const int N = 1000000;
const int a = 2.0;

static void init_x(float *x)
{
    for (int i = 0; i < N; i++) {
        x[i] = 1.0f;
    }
}

static void init_y(float *y)
{
    for (int i = 0; i < N; i++) {
        y[i] = 2.0f;
    }
}

static int check_result(float *y)
{
    float maxError = 0.0f;
    int errs = 0;
    for (int i = 0; i < N; i++) {
        if (abs(y[i] - 4.0f) > 0.01) {
            errs++;
            maxError = max(maxError, abs(y[i]-4.0f));
        }
    }
    if (errs > 0) {
        printf("%d errors, Max error: %f\n", errs, maxError);
    }
    return errs;
}

__global__
void saxpy(int n, float a, float *x, float *y)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  if (i < n) y[i] = a*x[i] + y[i];
}

int main(void)
{
    int errs = 0;

    int mpi_errno;
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This test require 2 processes\n");
    }

    float *x, *y, *d_x, *d_y;
    x = (float*)malloc(N*sizeof(float));
    y = (float*)malloc(N*sizeof(float));

    hipMalloc(&d_x, N*sizeof(float)); 
    hipMalloc(&d_y, N*sizeof(float));

    if (rank == 0) {
        init_x(x);
    } else if (rank == 1) {
        init_y(y);
    }

    // P0 send x to P1, P1 run saxpy and check

    if (rank == 0) {
        hipMemcpy(d_x, x, N*sizeof(float), hipMemcpyHostToDevice);
        mpi_errno = MPI_Send(d_x, N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD);
        assert(mpi_errno == MPI_SUCCESS);
    } else if (rank == 1) {
        hipMemcpy(d_y, y, N*sizeof(float), hipMemcpyHostToDevice);
        mpi_errno = MPI_Recv(d_x, N, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assert(mpi_errno == MPI_SUCCESS);

        saxpy<<<(N+255)/256, 256, 0>>>(N, a, d_x, d_y);

        hipMemcpy(y, d_y, N*sizeof(float), hipMemcpyDeviceToHost);
    }

    if (rank == 1) {
        errs = check_result(y);
        if (errs == 0) {
            printf("No Errors\n");
        }
    }

    hipFree(d_x);
    hipFree(d_y);
    free(x);
    free(y);

    MPI_Finalize();
}
