/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_Allreduce with MPI_IN_PLACE";
*/

void set_buf(int rank, int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        int val = rank + i;
        buf[i] = val;
    }
}

void check_buf(int size, int count, int *errs, int *buf)
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

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, i;
    int minsize = 2, count;
    MPI_Comm comm;
    void *buf_h, *buf;
    mtest_mem_type_e memtype;

    MTest_Init(&argc, &argv);
    MTestArgList *head = MTestArgListCreate(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank % 2 == 0)
        memtype = MTestArgListGetMemType(head, "evenmemtype");
    else
        memtype = MTestArgListGetMemType(head, "oddmemtype");

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);

        for (count = 1; count < 65000; count = count * 2) {
            MTestAlloc(count * sizeof(int), memtype, &buf_h, &buf, 0);

            set_buf(rank, count, buf_h);
            MTestCopyContent(buf_h, buf, count * sizeof(int), memtype);

            MPI_Allreduce(MPI_IN_PLACE, buf, count, MPI_INT, MPI_SUM, comm);

            MTestCopyContent(buf, buf_h, count * sizeof(int), memtype);
            check_buf(size, count, &errs, buf_h);

            MTestFree(memtype, buf_h, buf);
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
