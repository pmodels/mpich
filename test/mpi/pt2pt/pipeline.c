/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Send-Recv";
*/

int world_rank, world_size;

static void set_buf(int rank, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int val = rank + i;
        buf[i] = val;
    }
}

static void check_buf(int res, int count, int *errs, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int result = res + i;
        if (buf[i] != result) {
            (*errs)++;
            if ((*errs) < 10) {
                fprintf(stderr, "buf[%d] = %d expected %d\n", i, buf[i], result);
            }
        }
    }
}

static int sendrecv1(MPI_Comm comm)
{
    int errs = 0;
    int err;
    MPI_Aint sendcount, recvcount;
    MPI_Datatype sendtype, recvtype;
    struct mtest_obj send, recv;
    mtest_mem_type_e memtype = MTEST_MEM_TYPE__DEVICE;
    MPI_Request reqs[2];

    int count1 = 1024 * 1024 * 16;
    void *buf_h1, *buf1;
    MTestMalloc(count1 * sizeof(int), memtype, &buf_h1, &buf1, world_rank);

    int count2 = 1024;
    void *buf_h2, *buf2;
    MTestMalloc(count2 * sizeof(int), memtype, &buf_h2, &buf2, world_rank);

    if (world_rank == 0) {
        set_buf(1, count1, buf_h1);
        MTestCopyContent(buf_h1, buf1, count1 * sizeof(int), memtype);

        set_buf(2, count2, buf_h2);
        MTestCopyContent(buf_h2, buf2, count2 * sizeof(int), memtype);

        err = MPI_Isend((const char *) buf1, count1, MPI_INT, 1, 0, comm, &reqs[0]);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        err = MPI_Isend((const char *) buf2, count2, MPI_INT, 1, 0, comm, &reqs[1]);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        MPI_Status statuses[2];
        MPI_Waitall(2, reqs, statuses);
    } else {
        set_buf(10, count1, buf_h1);
        MTestCopyContent(buf_h1, buf1, count1 * sizeof(int), memtype);

        set_buf(20, count2, buf_h2);
        MTestCopyContent(buf_h2, buf2, count2 * sizeof(int), memtype);

        MPI_Status status;
        err = MPI_Recv((char *) buf1, count1, MPI_INT, 0, 0, comm, &status);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        MTestCopyContent(buf1, buf_h1, count1 * sizeof(int), memtype);
        check_buf(1, count1, &errs, buf_h1);

        err = MPI_Recv((char *) buf2, count2, MPI_INT, 0, 0, comm, &status);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        MTestCopyContent(buf2, buf_h2, count2 * sizeof(int), memtype);
        check_buf(2, count2, &errs, buf_h2);
    }
    MTestFree(memtype, buf_h1, buf1);
    MTestFree(memtype, buf_h2, buf2);

    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    if (world_size != 2) {
        printf("This test requires 2 processes\n");
        errs++;
        goto fn_exit;
    }

    errs += sendrecv1(MPI_COMM_WORLD);

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
