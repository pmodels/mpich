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
static char MTEST_Descrip[] = "Test MPI_Bcast";
*/

static void set_buf(int rank, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int val = rank + i;
        buf[i] = val;
    }
}

static void check_buf(int size, int count, int *errs, int *buf, int root)
{
    int i;
    for (i = 0; i < count; i++) {
        if (buf[i] != root + i) {
            (*errs)++;
            if ((*errs) < 10) {
                fprintf(stderr, "buf[%d] = %d expected %d\n", i, buf[i], root + i);
            }
        }
    }
}

static int test_bcast(mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int errs = 0;
    int rank, size;
    int root = 0;
    int reps = 0;
    int num_reps = 3;
    int minsize = 2, count;
    MPI_Comm comm;
    void *buf_h, *buf;
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

        for (count = 1; count < 32000; count = count * 2) {
            MTestMalloc(count * sizeof(int), memtype, &buf_h, &buf, rank);
            for (root = 0; root < size; root++) {
                for (reps = 0; reps < num_reps; reps++) {
                    set_buf(rank, count, buf_h);
                    MTestCopyContent(buf_h, buf, count * sizeof(int), memtype);

                    MPI_Bcast(buf, count, MPI_INT, root, comm);

                    MTestCopyContent(buf, buf_h, count * sizeof(int), memtype);
                    check_buf(size, count, &errs, buf_h, root);
                }
            }

            MTestFree(memtype, buf_h, buf);
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
        errs += test_bcast(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
