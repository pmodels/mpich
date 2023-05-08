/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#define NTHREADS 4

static void test_barrier(MPI_Comm comm);
static void test_bcast(MPI_Comm comm);
static void test_gather(MPI_Comm comm);
static void test_gatherv(MPI_Comm comm);
static void test_scatter(MPI_Comm comm);
static void test_scatterv(MPI_Comm comm);
static void test_allgather(MPI_Comm comm);
static void test_allgatherv(MPI_Comm comm);
static void test_alltoall(MPI_Comm comm);
static void test_alltoallv(MPI_Comm comm);
static void test_alltoallw(MPI_Comm comm);
static void test_reduce(MPI_Comm comm);
static void test_allreduce(MPI_Comm comm);
static void test_reduce_scatter(MPI_Comm comm);
static void test_reduce_scatter_block(MPI_Comm comm);
static void test_scan(MPI_Comm comm);
static void test_exscan(MPI_Comm comm);

static void test_bcast_c(MPI_Comm comm);
static void test_gather_c(MPI_Comm comm);
static void test_gatherv_c(MPI_Comm comm);
static void test_scatter_c(MPI_Comm comm);
static void test_scatterv_c(MPI_Comm comm);
static void test_allgather_c(MPI_Comm comm);
static void test_allgatherv_c(MPI_Comm comm);
static void test_alltoall_c(MPI_Comm comm);
static void test_alltoallv_c(MPI_Comm comm);
static void test_alltoallw_c(MPI_Comm comm);
static void test_reduce_c(MPI_Comm comm);
static void test_allreduce_c(MPI_Comm comm);
static void test_reduce_scatter_c(MPI_Comm comm);
static void test_reduce_scatter_block_c(MPI_Comm comm);
static void test_scan_c(MPI_Comm comm);
static void test_exscan_c(MPI_Comm comm);

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;

    MPIX_Threadcomm_start(comm);

    test_barrier(comm);
    test_bcast(comm);
    test_gather(comm);
    test_gatherv(comm);
    test_scatter(comm);
    test_scatterv(comm);
    test_allgather(comm);
    test_allgatherv(comm);
    test_alltoall(comm);
    test_alltoallv(comm);
    test_alltoallw(comm);
    test_reduce(comm);
    test_allreduce(comm);
    test_reduce_scatter(comm);
    test_reduce_scatter_block(comm);
    test_scan(comm);
    test_exscan(comm);

    test_bcast_c(comm);
    test_gather_c(comm);
    test_gatherv_c(comm);
    test_scatter_c(comm);
    test_scatterv_c(comm);
    test_allgather_c(comm);
    test_allgatherv_c(comm);
    test_alltoall_c(comm);
    test_alltoallv_c(comm);
    test_alltoallw_c(comm);
    test_reduce_c(comm);
    test_allreduce_c(comm);
    test_reduce_scatter_c(comm);
    test_reduce_scatter_block_c(comm);
    test_scan_c(comm);
    test_exscan_c(comm);

    MPIX_Threadcomm_finish(comm);
}

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(NULL, NULL);

    MPI_Comm comm;
    MPIX_Threadcomm_init(MPI_COMM_WORLD, NTHREADS, &comm);
    for (int i = 0; i < NTHREADS; i++) {
        MTest_Start_thread(test_thread, &comm);
    }
    MTest_Join_threads();
    MPIX_Threadcomm_free(&comm);

    MTest_Finalize(errs);
    return errs;
}

static void test_barrier(MPI_Comm comm)
{
    MPI_Barrier(comm);
}

static void test_bcast(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int n = 0;
    if (rank == root) {
        n = 42;
    }
    MPI_Bcast(&n, 1, MPI_INT, root, comm);

    if (n != 42) {
        printf("[%d] test_bcast: expect %d, got %d\n", rank, 42, n);
    }
}

static void test_gather(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];         /* variable length array */

    MPI_Gather(&rank, 1, MPI_INT, allranks, 1, MPI_INT, root, comm);

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            if (allranks[i] != i) {
                printf("[%d] test_gather: expect allranks[%d] %d, got %d\n",
                       rank, i, i, allranks[i]);
            }
        }
    }
}

static void test_gatherv(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size], counts[size], displs[size];

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            counts[i] = 1;
            displs[i] = i;
        }
    } else {
        /* set to invalid values */
        for (int i = 0; i < size; i++) {
            counts[i] = -1;
            displs[i] = -1;
        }
    }

    MPI_Gatherv(&rank, 1, MPI_INT, allranks, counts, displs, MPI_INT, root, comm);

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            if (allranks[i] != i) {
                printf("[%d] test_gather: expect allranks[%d] %d, got %d\n",
                       rank, i, i, allranks[i]);
            }
        }
    }
}

static void test_scatter(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    if (rank == root) {
        for (int i = 0; i < size; i++) {
            data_out[i] = i;
        }
    }

    int data_in;
    MPI_Scatter(data_out, 1, MPI_INT, &data_in, 1, MPI_INT, root, comm);

    if (data_in != rank) {
        printf("[%d] test_scatter: expect %d, got %d\n", rank, rank, data_in);
    }
}

static void test_scatterv(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], counts[size], displs[size];

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            data_out[i] = i;
            counts[i] = 1;
            displs[i] = i;
        }
    } else {
        /* set to invalid values */
        for (int i = 0; i < size; i++) {
            counts[i] = -1;
            displs[i] = -1;
        }
    }

    int data_in;
    MPI_Scatterv(&data_out, counts, displs, MPI_INT, &data_in, 1, MPI_INT, root, comm);

    if (data_in != rank) {
        printf("[%d] test_scatterv: expect %d, got %d\n", rank, rank, data_in);
    }
}

static void test_allgather(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];

    MPI_Allgather(&rank, 1, MPI_INT, allranks, 1, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (allranks[i] != i) {
            printf("[%d] test_gather: expect allranks[%d] %d, got %d\n", rank, i, i, allranks[i]);
        }
    }
}

static void test_allgatherv(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size], counts[size], displs[size];

    for (int i = 0; i < size; i++) {
        counts[i] = 1;
        displs[i] = i;
    }

    MPI_Allgatherv(&rank, 1, MPI_INT, allranks, counts, displs, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (allranks[i] != i) {
            printf("[%d] test_gatherv: expect allranks[%d] %d, got %d\n", rank, i, i, allranks[i]);
        }
    }
}

static void test_alltoall(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
    }

    MPI_Alltoall(data_out, 1, MPI_INT, data_in, 1, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoall: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_alltoallv(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    int counts[size], displs[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
        counts[i] = 1;
        displs[i] = i;
    }

    MPI_Alltoallv(data_out, counts, displs, MPI_INT, data_in, counts, displs, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoallv: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_alltoallw(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    int counts[size], displs[size];
    MPI_Datatype types[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
        counts[i] = 1;
        displs[i] = i * sizeof(int);
        types[i] = MPI_INT;
    }

    MPI_Alltoallw(data_out, counts, displs, types, data_in, counts, displs, types, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoallw: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_reduce(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Reduce(&rank, &sum, 1, MPI_INT, MPI_SUM, root, comm);

    if (rank == root) {
        if (sum != size * (size - 1) / 2) {
            printf("[%d] test_reduce: expect %d, got %d\n", rank, size * (size - 1) / 2, sum);
        }
    }
}

static void test_allreduce(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Allreduce(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (sum != size * (size - 1) / 2) {
        printf("[%d] test_allreduce: expect %d, got %d\n", rank, size * (size - 1) / 2, sum);
    }
}

static void test_reduce_scatter(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    int counts[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i;
        counts[i] = 1;
    }

    int data_in;
    MPI_Reduce_scatter(data_out, &data_in, counts, MPI_INT, MPI_SUM, comm);

    if (data_in != rank * size) {
        printf("[%d] test_reduce_scatter_block: expect %d, got %d\n", rank, rank * size, data_in);
    }
}

static void test_reduce_scatter_block(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i;
    }

    int data_in;
    MPI_Reduce_scatter_block(data_out, &data_in, 1, MPI_INT, MPI_SUM, comm);

    if (data_in != rank * size) {
        printf("[%d] test_reduce_scatter_block: expect %d, got %d\n", rank, rank * size, data_in);
    }
}

static void test_scan(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Scan(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (sum != rank * (rank + 1) / 2) {
        printf("[%d] test_scan: expect %d, got %d\n", rank, rank * (rank - 1) / 2, sum);
    }
}

static void test_exscan(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Exscan(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (rank > 0) {
        if (sum != (rank - 1) * rank / 2) {
            printf("[%d] test_exscan: expect %d, got %d\n", rank, (rank - 1) * rank / 2, sum);
        }
    }
}

static void test_bcast_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int n = 0;
    if (rank == root) {
        n = 42;
    }
    MPI_Bcast_c(&n, 1, MPI_INT, root, comm);

    if (n != 42) {
        printf("[%d] test_bcast: expect %d, got %d\n", rank, 42, n);
    }
}

static void test_gather_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];         /* variable length array */

    MPI_Gather_c(&rank, 1, MPI_INT, allranks, 1, MPI_INT, root, comm);

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            if (allranks[i] != i) {
                printf("[%d] test_gather: expect allranks[%d] %d, got %d\n",
                       rank, i, i, allranks[i]);
            }
        }
    }
}

static void test_gatherv_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];
    MPI_Count counts[size], displs[size];

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            counts[i] = 1;
            displs[i] = i;
        }
    } else {
        /* set to invalid values */
        for (int i = 0; i < size; i++) {
            counts[i] = -1;
            displs[i] = -1;
        }
    }

    MPI_Gatherv_c(&rank, 1, MPI_INT, allranks, counts, displs, MPI_INT, root, comm);

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            if (allranks[i] != i) {
                printf("[%d] test_gather: expect allranks[%d] %d, got %d\n",
                       rank, i, i, allranks[i]);
            }
        }
    }
}

static void test_scatter_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    if (rank == root) {
        for (int i = 0; i < size; i++) {
            data_out[i] = i;
        }
    }

    int data_in;
    MPI_Scatter_c(data_out, 1, MPI_INT, &data_in, 1, MPI_INT, root, comm);

    if (data_in != rank) {
        printf("[%d] test_scatter: expect %d, got %d\n", rank, rank, data_in);
    }
}

static void test_scatterv_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    MPI_Count counts[size], displs[size];

    if (rank == root) {
        for (int i = 0; i < size; i++) {
            data_out[i] = i;
            counts[i] = 1;
            displs[i] = i;
        }
    } else {
        /* set to invalid values */
        for (int i = 0; i < size; i++) {
            counts[i] = -1;
            displs[i] = -1;
        }
    }

    int data_in;
    MPI_Scatterv_c(&data_out, counts, displs, MPI_INT, &data_in, 1, MPI_INT, root, comm);

    if (data_in != rank) {
        printf("[%d] test_scatterv: expect %d, got %d\n", rank, rank, data_in);
    }
}

static void test_allgather_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];

    MPI_Allgather_c(&rank, 1, MPI_INT, allranks, 1, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (allranks[i] != i) {
            printf("[%d] test_gather: expect allranks[%d] %d, got %d\n", rank, i, i, allranks[i]);
        }
    }
}

static void test_allgatherv_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int allranks[size];
    MPI_Count counts[size], displs[size];

    for (int i = 0; i < size; i++) {
        counts[i] = 1;
        displs[i] = i;
    }

    MPI_Allgatherv_c(&rank, 1, MPI_INT, allranks, counts, displs, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (allranks[i] != i) {
            printf("[%d] test_gatherv: expect allranks[%d] %d, got %d\n", rank, i, i, allranks[i]);
        }
    }
}

static void test_alltoall_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
    }

    MPI_Alltoall_c(data_out, 1, MPI_INT, data_in, 1, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoall: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_alltoallv_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    MPI_Count counts[size], displs[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
        counts[i] = 1;
        displs[i] = i;
    }

    MPI_Alltoallv_c(data_out, counts, displs, MPI_INT, data_in, counts, displs, MPI_INT, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoallv: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_alltoallw_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size], data_in[size];
    MPI_Count counts[size], displs[size];
    MPI_Datatype types[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i * 1000 + rank;
        data_in[i] = -1;
        counts[i] = 1;
        displs[i] = i * sizeof(int);
        types[i] = MPI_INT;
    }

    MPI_Alltoallw_c(data_out, counts, displs, types, data_in, counts, displs, types, comm);

    for (int i = 0; i < size; i++) {
        if (data_in[i] != rank * 1000 + i) {
            printf("[%d] test_alltoallw: expect %d, got %d\n", rank, rank * 1000 + i, data_in[i]);
        }
    }
}

static void test_reduce_c(MPI_Comm comm)
{
    int root = 0;
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Reduce_c(&rank, &sum, 1, MPI_INT, MPI_SUM, root, comm);

    if (rank == root) {
        if (sum != size * (size - 1) / 2) {
            printf("[%d] test_reduce: expect %d, got %d\n", rank, size * (size - 1) / 2, sum);
        }
    }
}

static void test_allreduce_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Allreduce_c(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (sum != size * (size - 1) / 2) {
        printf("[%d] test_allreduce: expect %d, got %d\n", rank, size * (size - 1) / 2, sum);
    }
}

static void test_reduce_scatter_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    MPI_Count counts[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i;
        counts[i] = 1;
    }

    int data_in;
    MPI_Reduce_scatter_c(data_out, &data_in, counts, MPI_INT, MPI_SUM, comm);

    if (data_in != rank * size) {
        printf("[%d] test_reduce_scatter_block: expect %d, got %d\n", rank, rank * size, data_in);
    }
}

static void test_reduce_scatter_block_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int data_out[size];
    for (int i = 0; i < size; i++) {
        data_out[i] = i;
    }

    int data_in;
    MPI_Reduce_scatter_block_c(data_out, &data_in, 1, MPI_INT, MPI_SUM, comm);

    if (data_in != rank * size) {
        printf("[%d] test_reduce_scatter_block: expect %d, got %d\n", rank, rank * size, data_in);
    }
}

static void test_scan_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Scan_c(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (sum != rank * (rank + 1) / 2) {
        printf("[%d] test_scan: expect %d, got %d\n", rank, rank * (rank - 1) / 2, sum);
    }
}

static void test_exscan_c(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Exscan_c(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (rank > 0) {
        if (sum != (rank - 1) * rank / 2) {
            printf("[%d] test_exscan: expect %d, got %d\n", rank, (rank - 1) * rank / 2, sum);
        }
    }
}
