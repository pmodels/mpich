/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <assert.h>

const int N = 1000000;
const int a = 2.0;
const float x_val = 1.0f;
const float y_val = 2.0f;
const float exp_result = 4.0f;

static int check_result(float *y)
{
    float maxError = 0.0f;
    int errs = 0;
    for (int i = 0; i < N; i++) {
        if (abs(y[i] - exp_result) > 0.01) {
            errs++;
            maxError = max(maxError, abs(y[i] - exp_result));
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

static int need_progress_thread = 0;
static void parse_args(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-progress-thread") == 0) {
            need_progress_thread = 1;
        }
    }
}

int main(int argc, char **argv)
{
    int errs = 0;

    parse_args(argc, argv);

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

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "type", "cudaStream_t");
    MPIX_Info_set_hex(info, "value", &stream, sizeof(stream));

    MPIX_Stream mpi_stream;
    MPIX_Stream_create(info, &mpi_stream);

    MPI_Info_free(&info);

    if (need_progress_thread) {
        MPIX_Start_progress_thread(mpi_stream);
    }

    MPI_Comm stream_comm;
    MPIX_Stream_comm_create(MPI_COMM_WORLD, mpi_stream, &stream_comm);

    /* Rank 0 sends x data to Rank 1, Rank 1 performs a * x + y and checks result */
    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            x[i] = x_val;
        }
        cudaMemcpyAsync(d_x, x, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Send_enqueue(d_x, N, MPI_FLOAT, 1, 0, stream_comm);
        assert(mpi_errno == MPI_SUCCESS);
    } else if (rank == 1) {
        for (int i = 0; i < N; i++) {
            y[i] = y_val;
        }
        cudaMemcpyAsync(d_y, y, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Recv_enqueue(d_x, N, MPI_FLOAT, 0, 0, stream_comm, MPI_STATUS_IGNORE);
        assert(mpi_errno == MPI_SUCCESS);

        saxpy<<<(N+255)/256, 256, 0, stream>>>(N, a, d_x, d_y);

        cudaMemcpyAsync(y, d_y, N*sizeof(float), cudaMemcpyDeviceToHost, stream);

        cudaStreamSynchronize(stream);
        errs += check_result(y);
    }

    /* Test again with MPIX_Isend_enqueue and MPIX_Wait_enqueue */
    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            x[i] = x_val;
        }
        /* we are directly sending from x in this test */
        MPI_Request req;
        mpi_errno = MPIX_Isend_enqueue(x, N, MPI_FLOAT, 1, 0, stream_comm, &req);
        assert(mpi_errno == MPI_SUCCESS);
        /* req won't reset to MPI_REQUEST_NULL, but user shouldn't use it afterward */
        mpi_errno = MPIX_Wait_enqueue(&req, MPI_STATUS_IGNORE);
        assert(mpi_errno == MPI_SUCCESS);
    } else if (rank == 1) {
        /* reset d_x, d_y */
        for (int i = 0; i < N; i++) {
            x[i] = 0.0;
            y[i] = y_val;
        }
        cudaMemcpyAsync(d_x, x, N*sizeof(float), cudaMemcpyHostToDevice, stream);
        cudaMemcpyAsync(d_y, y, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        MPI_Request req;
        mpi_errno = MPIX_Irecv_enqueue(d_x, N, MPI_FLOAT, 0, 0, stream_comm, &req);
        assert(mpi_errno == MPI_SUCCESS);
        mpi_errno = MPIX_Waitall_enqueue(1, &req, MPI_STATUSES_IGNORE);
        assert(mpi_errno == MPI_SUCCESS);

        saxpy<<<(N+255)/256, 256, 0, stream>>>(N, a, d_x, d_y);

        cudaMemcpyAsync(y, d_y, N*sizeof(float), cudaMemcpyDeviceToHost, stream);

        cudaStreamSynchronize(stream);
        errs += check_result(y);
    }

    if (need_progress_thread) {
        MPIX_Stop_progress_thread(mpi_stream);
    }

    MPI_Comm_free(&stream_comm);
    MPIX_Stream_free(&mpi_stream);

    cudaFree(d_x);
    cudaFree(d_y);
    free(x);
    free(y);

    cudaStreamDestroy(stream);
    MPI_Finalize();

    if (rank == 1 && errs == 0) {
        printf("No Errors\n");
    }
    return errs;
}
