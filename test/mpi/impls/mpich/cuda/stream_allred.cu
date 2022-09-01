/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <assert.h>

int rank, size;

#define CHECK_RESULT(i, result, expected, msg) \
    do { \
        if (result != expected) { \
            printf("%s: i = %d, expect %d, got %d\n", msg, i, expected, result); \
            errs++; \
        } \
    } while (0)

static int test_1(MPI_Comm comm, cudaStream_t stream, bool do_inplace)
{
    int errs = 0;
    int mpi_errno;
#define N 10    
    /* TEST 1 - MPI_INT */
    int buf[N];
    void *d_buf, *d_result_buf;
    cudaMalloc(&d_buf, sizeof(buf));
    cudaMalloc(&d_result_buf, sizeof(buf));

    for (int i = 0; i < N; i++) {
        buf[i] = rank;
    }

    int expected_sum = size * (size - 1) / 2;

    const void *sendbuf;
    void *recvbuf;
    if (do_inplace) {
        cudaMemcpyAsync(d_result_buf, buf, sizeof(buf), cudaMemcpyHostToDevice, stream);
        sendbuf = MPI_IN_PLACE;
        recvbuf = d_result_buf;
    } else {
        cudaMemcpyAsync(d_buf, buf, sizeof(buf), cudaMemcpyHostToDevice, stream);
        sendbuf = d_buf;
        recvbuf = d_result_buf;
    }
    mpi_errno = MPIX_Allreduce_enqueue(sendbuf, recvbuf, N, MPI_INT, MPI_SUM, comm);
    assert(mpi_errno == MPI_SUCCESS);
    cudaMemcpyAsync(buf, d_result_buf, sizeof(buf), cudaMemcpyDeviceToHost, stream);
    cudaStreamSynchronize(stream);

    cudaFree(d_buf);
    cudaFree(d_result_buf);

    const char *test_name = "Test 1";
    if (do_inplace) {
        test_name = "TEST 1 (MPI_IN_PLACE)";
    }
    for (int i = 0; i < N; i++) {
        CHECK_RESULT(i, buf[i], expected_sum, test_name);
    }
#undef N
    return errs;
}

static int test_2(MPI_Comm comm, cudaStream_t stream, bool do_inplace)
{
    int errs = 0;
    int mpi_errno;
#define N 10    
    /* TEST 2 - Pairtype (non-contig) */
    struct {
        short a;
        int b;
    } buf[N];
    void *d_buf, *d_result_buf;
    cudaMalloc(&d_buf, sizeof(buf));
    cudaMalloc(&d_result_buf, sizeof(buf));

    for(int i = 0; i < N; i++) {
        /* MINLOC result should be {0, i % size} */
        if (i % size == rank) {
            buf[i].a = 0;
        } else {
            buf[i].a = rank + 1;
        }
        buf[i].b = rank;
    }

    const void *sendbuf;
    void *recvbuf;
    if (do_inplace) {
        cudaMemcpyAsync(d_result_buf, buf, sizeof(buf), cudaMemcpyHostToDevice, stream);
        sendbuf = MPI_IN_PLACE;
        recvbuf = d_result_buf;
    } else {
        cudaMemcpyAsync(d_buf, buf, sizeof(buf), cudaMemcpyHostToDevice, stream);
        sendbuf = d_buf;
        recvbuf = d_result_buf;
    }
    mpi_errno = MPIX_Allreduce_enqueue(sendbuf, recvbuf, N, MPI_SHORT_INT, MPI_MINLOC, comm);
    assert(mpi_errno == MPI_SUCCESS);
    cudaMemcpyAsync(buf, d_result_buf, sizeof(buf), cudaMemcpyDeviceToHost, stream);
    cudaStreamSynchronize(stream);

    const char *test_name = "Test 2";
    if (do_inplace) {
        test_name = "TEST 2 (MPI_IN_PLACE)";
    }
    for (int i = 0; i < N; i++) {
        CHECK_RESULT(i, buf[i].a, 0, test_name);
        CHECK_RESULT(i, buf[i].b, i % size, test_name);
    }
#undef N
    return errs;
}

int main(void)
{
    int errs = 0;

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "type", "cudaStream_t");
    MPIX_Info_set_hex(info, "value", &stream, sizeof(stream));

    MPIX_Stream mpi_stream;
    MPIX_Stream_create(info, &mpi_stream);

    MPI_Info_free(&info);

    MPI_Comm stream_comm;
    MPIX_Stream_comm_create(MPI_COMM_WORLD, mpi_stream, &stream_comm);

    errs += test_1(stream_comm, stream, false);
    errs += test_1(stream_comm, stream, true);  /* MPI_IN_PLACE */
    errs += test_2(stream_comm, stream, false);
    errs += test_2(stream_comm, stream, true);  /* MPI_IN_PLACE */

    /* clean up */
    MPI_Comm_free(&stream_comm);
    MPIX_Stream_free(&mpi_stream);

    cudaStreamDestroy(stream);

    int tot_errs;
    MPI_Reduce(&errs, &tot_errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (tot_errs == 0) {
            printf("No Errors\n");
        } else {
            printf("%d Errors\n", tot_errs);
        }
    }

    MPI_Finalize();
    return errs;
}
