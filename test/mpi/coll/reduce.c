/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mtest_dtp.h"

/*
static char MTEST_Descrip[] = "A simple test of Reduce with all choices of root process";
*/

static void set_send_buf(int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = i;
    }
}

static void set_recv_buf(int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = -1;
    }
}

static void check_buf(int rank, int size, int count, int *errs, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int result = i * size;
        if (buf[i] != result) {
            (*errs)++;
            if ((*errs) < 10) {
                fprintf(stderr, "buf[%d] = %d expected %d\n", i, buf[i], result);
            }
        }
    }
}

static int test_reduce(mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int errs = 0;
    int rank, size, root;
    void *sendbuf, *recvbuf, *sendbuf_h, *recvbuf_h;
    int minsize = 2, count;
    MPI_Comm comm;
    mtest_mem_type_e memtype;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank % 2 == 0)
        memtype = evenmem;
    else
        memtype = oddmem;

    if (rank == 0) {
        MTestPrintfMsg(1, "./reduce -evenmemtype=%s -oddmemtype=%s\n",
                       MTest_memtype_name(evenmem), MTest_memtype_name(oddmem));
    }

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        for (count = 1; count < 130000; count = count * 2) {
            MTestMalloc(count * sizeof(int), memtype, &recvbuf_h, &recvbuf, rank);
            MTestMalloc(count * sizeof(int), memtype, &sendbuf_h, &sendbuf, rank);

            for (root = 0; root < size; root++) {
                set_send_buf(count, sendbuf_h);
                set_recv_buf(count, recvbuf_h);
                MTestCopyContent(sendbuf_h, sendbuf, count * sizeof(int), memtype);
                MTestCopyContent(recvbuf_h, recvbuf, count * sizeof(int), memtype);

                MPI_Reduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, root, comm);

                if (rank == root) {
                    MTestCopyContent(recvbuf, recvbuf_h, count * sizeof(int), memtype);
                    check_buf(rank, size, count, &errs, recvbuf_h);
                }

                /* test again using MPI_IN_PLACE */
                if (rank == root) {
                    set_send_buf(count, recvbuf_h);
                    MTestCopyContent(recvbuf_h, recvbuf, count * sizeof(int), memtype);
                    MPI_Reduce(MPI_IN_PLACE, recvbuf, count, MPI_INT, MPI_SUM, root, comm);
                } else {
                    set_send_buf(count, sendbuf_h);
                    MTestCopyContent(sendbuf_h, sendbuf, count * sizeof(int), memtype);
                    MPI_Reduce(sendbuf, NULL, count, MPI_INT, MPI_SUM, root, comm);
                }

                if (rank == root) {
                    MTestCopyContent(recvbuf, recvbuf_h, count * sizeof(int), memtype);
                    check_buf(rank, size, count, &errs, recvbuf_h);
                }
            }
            MTestFree(memtype, sendbuf_h, sendbuf);
            MTestFree(memtype, recvbuf_h, recvbuf);
        }
        MTestFreeComm(&comm);
    }
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_COLL_NOCOUNT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += test_reduce(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
