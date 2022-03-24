/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

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

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    int mpi_errno;
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This test require 2 processes\n");
        exit(1);
    }

    float *x, *y, *d_x, *d_y;
    x = (float*)malloc(N*sizeof(float));
    y = (float*)malloc(N*sizeof(float));

    cudaMalloc(&d_x, N*sizeof(float));
    cudaMalloc(&d_y, N*sizeof(float));

    if (rank == 0) {
        init_x(x);
    } else if (rank == 1) {
        init_y(y);
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "type", "cudaStream_t");
    MPIX_Info_set_hex(info, "value", &stream, sizeof(stream));

    MPIX_Stream mpi_stream;
    MPIX_Stream_create(info, &mpi_stream);

    MPI_Info_free(&info);

    MPI_Comm stream_comm;
    MPIX_Stream_comm_create(MPI_COMM_WORLD, mpi_stream, &stream_comm);

    /* Rank 0 sends x data to Rank 1, Rank 1 performs a * x + y and checks result */
    if (rank == 0) {
        cudaMemcpyAsync(d_x, x, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Send_enqueue(d_x, N, MPI_FLOAT, 1, 0, stream_comm);
        assert(mpi_errno == MPI_SUCCESS);

        cudaStreamSynchronize(stream);
    } else if (rank == 1) {
        cudaMemcpyAsync(d_y, y, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Recv_enqueue(d_x, N, MPI_FLOAT, 0, 0, stream_comm, MPI_STATUS_IGNORE);
        assert(mpi_errno == MPI_SUCCESS);

        saxpy<<<(N+255)/256, 256, 0, stream>>>(N, a, d_x, d_y);

        cudaMemcpyAsync(y, d_y, N*sizeof(float), cudaMemcpyDeviceToHost, stream);

        cudaStreamSynchronize(stream);
    }

    if (rank == 1) {
        int errs = check_result(y);
        if (errs == 0) {
            printf("No Errors\n");
        }
    }

    MPI_Comm_free(&stream_comm);
    MPIX_Stream_free(&mpi_stream);

    cudaFree(d_x);
    cudaFree(d_y);
    free(x);
    free(y);

    cudaStreamDestroy(stream);
    MPI_Finalize();
    return errs;
}
