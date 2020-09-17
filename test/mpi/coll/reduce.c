/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "A simple test of Reduce with all choices of root process";
*/

void set_send_buf(int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = i;
    }
}

void set_recv_buf(int count, int *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        buf[i] = -1;
    }
}

void check_buf(int rank, int size, int count, int *errs, int *buf)
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

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, root, i;
    void *sendbuf, *recvbuf, *sendbuf_h, *recvbuf_h;
    int minsize = 2, count;
    MPI_Comm comm;
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
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        for (count = 1; count < 130000; count = count * 2) {
            MTestAlloc(count * sizeof(int), memtype, &recvbuf_h, &recvbuf, 0);
            MTestAlloc(count * sizeof(int), memtype, &sendbuf_h, &sendbuf, 0);

            for (root = 0; root < size; root++) {
                set_send_buf(count, sendbuf_h);
                set_recv_buf(count, recvbuf_h);
                MTestCopyContent(sendbuf_h, sendbuf, count * sizeof(int), memtype);
                MTestCopyContent(recvbuf_h, recvbuf, count * sizeof(int), memtype);

                MPI_Reduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, root, comm);
                MTestCopyContent(recvbuf, recvbuf_h, count * sizeof(int), memtype);

                if (rank == root) {
                    check_buf(rank, size, count, &errs, recvbuf_h);
                }
            }
            MTestFree(memtype, sendbuf_h, sendbuf);
            MTestFree(memtype, recvbuf_h, recvbuf);
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
