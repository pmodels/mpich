/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mtest_dtp.h"

#ifdef MULTI_TESTS
#define run coll_allred2
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test MPI_Allreduce with MPI_IN_PLACE";
*/

static void set_buf(int rank, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int val = rank + i;
        buf[i] = val;
    }
}

static void check_buf(int size, int count, int *errs, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int result = i * size + (size * (size - 1)) / 2;
        if (buf[i] != result) {
            (*errs)++;
            if ((*errs) < 10) {
                fprintf(stderr, "buf[%d] = %d expected %d\n", i, buf[i], result);
            }
        }
    }
}

static int test_allred(mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int errs = 0;
    int rank, size;
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

        MTestMalloc(32768 * sizeof(int), memtype, &buf_h, &buf, rank);

        for (count = 1; count < 65000; count = count * 8) {
            set_buf(rank, count, buf_h);
            MTestCopyContent(buf_h, buf, count * sizeof(int), memtype);

            MPI_Allreduce(MPI_IN_PLACE, buf, count, MPI_INT, MPI_SUM, comm);

            MTestCopyContent(buf, buf_h, count * sizeof(int), memtype);
            check_buf(size, count, &errs, buf_h);
        }

        MTestFree(memtype, buf_h, buf);
        MTestFreeComm(&comm);
    }
    return errs;
}

int run(const char *arg)
{
    int errs = 0;

    struct dtp_args dtp_args;
    dtp_args_init_arg(&dtp_args, MTEST_COLL_NOCOUNT, arg);
    while (dtp_args_get_next(&dtp_args)) {
        errs += test_allred(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    return errs;
}
