/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Test of reduce_scatter_block.
 *
 * Checks that non-commutative operations are not commuted and that
 * all of the operations are performed.
 *
 * Can be called with any number of processors.
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_red_scat_block2
int run(const char *arg);
#endif

static int err = 0;

/* left(x,y) ==> x */
static void left(void *a, void *b, int *count, MPI_Datatype * type)
{
    int *in = a;
    int *inout = b;
    int i;

    for (i = 0; i < *count; ++i) {
        if (in[i] > inout[i])
            ++err;
        inout[i] = in[i];
    }
}

/* right(x,y) ==> y */
static void right(void *a, void *b, int *count, MPI_Datatype * type)
{
    int *in = a;
    int *inout = b;
    int i;

    for (i = 0; i < *count; ++i) {
        if (in[i] > inout[i])
            ++err;
        inout[i] = inout[i];
    }
}

/* Just performs a simple sum but can be marked as non-commutative to
   potentially trigger different logic in the implementation. */
static void nc_sum(void *a, void *b, int *count, MPI_Datatype * type)
{
    int *in = a;
    int *inout = b;
    int i;

    for (i = 0; i < *count; ++i) {
        inout[i] = in[i] + inout[i];
    }
}

#define MAX_BLOCK_SIZE 256

int run(const char *arg)
{
    int *sendbuf;
    int block_size;
    int *recvbuf;
    int size, rank, i;
    MPI_Comm comm;
    MPI_Op left_op, right_op, nc_sum_op;

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* MPI_Reduce_scatter block was added in MPI-2.2 */

    MPI_Op_create(&left, 0 /*non-commutative */ , &left_op);
    MPI_Op_create(&right, 0 /*non-commutative */ , &right_op);
    MPI_Op_create(&nc_sum, 0 /*non-commutative */ , &nc_sum_op);

    for (block_size = 1; block_size < MAX_BLOCK_SIZE; block_size *= 2) {
        sendbuf = (int *) malloc(block_size * size * sizeof(int));
        recvbuf = malloc(block_size * sizeof(int));

        for (i = 0; i < (size * block_size); i++)
            sendbuf[i] = rank + i;
        for (i = 0; i < block_size; i++)
            recvbuf[i] = 0xdeadbeef;

        MPI_Reduce_scatter_block(sendbuf, recvbuf, block_size, MPI_INT, left_op, comm);
        for (i = 0; i < block_size; ++i)
            if (recvbuf[i] != (rank * block_size + i))
                ++err;

        MPI_Reduce_scatter_block(sendbuf, recvbuf, block_size, MPI_INT, right_op, comm);
        for (i = 0; i < block_size; ++i)
            if (recvbuf[i] != ((size - 1) + (rank * block_size) + i))
                ++err;

        MPI_Reduce_scatter_block(sendbuf, recvbuf, block_size, MPI_INT, nc_sum_op, comm);
        for (i = 0; i < block_size; ++i) {
            int x = rank * block_size + i;
            if (recvbuf[i] != (size * x + (size - 1) * size / 2))
                ++err;
        }

        free(recvbuf);
        free(sendbuf);
    }

    MPI_Op_free(&left_op);
    MPI_Op_free(&right_op);
    MPI_Op_free(&nc_sum_op);
#endif

    return err;
}
