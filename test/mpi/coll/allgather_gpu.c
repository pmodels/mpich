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
static char MTEST_Descrip[] = "Test MPI_Alltoall";
*/

static void set_sendbuf(int rank, int size, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = rank * count + i;
    }
}

static void set_recvbuf(int size, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = -1;
    }
}


static void check_buf(int rank, int size, int count, int *errs, int *buf)
{
    int i, j;
    for (i = 0; i < count * size; i++) {
        if (buf[i] != i) {
            (*errs)++;
            if ((*errs) < 10) {
                fprintf(stderr, "Error with communicator %s and size=%d count=%d\n",
                        MTestGetIntracommName(), size, count);
                fprintf(stderr, "recvbuf[%d,%d] = %d, should be %d\n", j, i, buf[i], i);
            }
        }
    }
}

static int test_allgather(mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int errs = 0;
    int rank, size;
    int reps = 0;
    int num_reps = 2;
    int minsize = 2, count;
    MPI_Comm comm;
    void *sendbuf_h, *sendbuf;
    void *recvbuf_h, *recvbuf;
    mtest_mem_type_e memtype;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank % 2 == 0)
        memtype = evenmem;
    else
        memtype = oddmem;

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);

        for (count = 1; count < 65000; count = count * 2) {
            MTestMalloc(count * size * sizeof(int), memtype, &sendbuf_h, &sendbuf, rank);
            MTestMalloc(count * size * sizeof(int), memtype, &recvbuf_h, &recvbuf, rank);

            for (reps = 0; reps < num_reps; reps++) {
                set_sendbuf(rank, size, count, sendbuf_h);
                MTestCopyContent(sendbuf_h, sendbuf, count * size * sizeof(int), memtype);
                set_recvbuf(size, count, recvbuf_h);
                MTestCopyContent(recvbuf_h, recvbuf, count * size * sizeof(int), memtype);

                MPI_Allgather(sendbuf, count, MPI_INT, recvbuf, count, MPI_INT, comm);

                MTestCopyContent(recvbuf, recvbuf_h, count * size * sizeof(int), memtype);
                check_buf(rank, size, count, &errs, recvbuf_h);
            }

            MTestFree(memtype, sendbuf_h, sendbuf);
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
        errs += test_allgather(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
