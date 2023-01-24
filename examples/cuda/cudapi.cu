/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <math.h>

#define THREADS_PER_BLOCK 256

__device__ double f(double a)
{
    return (4.0 / (1.0 + a * a));
}

__global__ void do_sum(int n, double h, int stride, double *sum) {
    int idx = 1 + (blockDim.x * blockIdx.x + threadIdx.x) + stride;
    __shared__ double block_sum;

    if (threadIdx.x == 0) {
        block_sum = 0.0;
    }
    __syncthreads();

    /* compute rectangles and add to block sum */
    if (idx <= n) {
        double x = h * ((double) idx - 0.5);
        atomicAdd(&block_sum, f(x));
    }

    /* add block sum to total */
    __syncthreads();
    if (threadIdx.x == 0) {
        atomicAdd(sum, block_sum * h);
    }
}

int main(int argc, char *argv[])
{
    int n, myid, numprocs;
    double PI25DT = 3.141592653589793238462643;
    double pi, h;
    double *sum;
    double startwtime = 0.0, endwtime;
    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Get_processor_name(processor_name, &namelen);

    fprintf(stdout, "Process %d of %d is on %s\n", myid, numprocs, processor_name);
    fflush(stdout);

    cudaMalloc((void **)&sum, sizeof(double));

    n = 10000;
    if (myid == 0)
        startwtime = MPI_Wtime();

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    h = 1.0 / (double) n;
    int blocks = (n + (THREADS_PER_BLOCK * numprocs - 1)) / (THREADS_PER_BLOCK * numprocs);
    int stride = blocks * THREADS_PER_BLOCK * myid;

    /* compute partial sum using the GPU */
    do_sum<<<blocks, THREADS_PER_BLOCK>>>(n, h, stride, sum);
    cudaDeviceSynchronize();

    MPI_Reduce(sum, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myid == 0) {
        endwtime = MPI_Wtime();
        printf("pi is approximately %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        printf("wall clock time = %f\n", endwtime - startwtime);
        fflush(stdout);
    }

    cudaFree(sum);

    MPI_Finalize();
    return 0;
}
