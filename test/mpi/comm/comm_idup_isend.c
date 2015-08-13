#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

#define ITERS 4

int main(int argc, char **argv)
{
    int errs = 0;
    int i, j;
    int rank, size, rsize;
    int in[ITERS], out[ITERS], sol[ITERS], cnt;
    int isLeft;
    MPI_Comm newcomm[ITERS], testcomm;
    MPI_Request *sreq;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    sreq = (MPI_Request *) malloc(sizeof(MPI_Request) * (size + 1) * ITERS);

    while (MTestGetIntracommGeneral(&testcomm, 1, 1)) {
        if (testcomm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(testcomm, &rank);
        MPI_Comm_size(testcomm, &size);
        cnt = 0;
        for (j = 0; j < ITERS; j++) {
            if (rank == 0) {
                out[j] = 815;
                in[j] = 815;
                sol[j] = 815;
                for (i = 1; i < size; i++)
                    MPI_Isend(&out[j], 1, MPI_INT, i, 0, testcomm, &sreq[cnt++]);
                MPI_Comm_idup(testcomm, &newcomm[j], &sreq[cnt++]);
            }
            else {
                out[j] = 0;
                in[j] = 0;
                sol[j] = 815;
                MPI_Comm_idup(testcomm, &newcomm[j], &sreq[cnt++]);
                MPI_Irecv(&in[j], 1, MPI_INT, 0, 0, testcomm, &sreq[cnt++]);
            }
        }
        MPI_Waitall(cnt, sreq, MPI_STATUS_IGNORE);

        for (j = 0; j < ITERS; j++) {
            if (sol[j] != in[j])
                errs++;
            errs += MTestTestComm(newcomm[j]);
            MPI_Comm_free(&newcomm[j]);
        }
        MTestFreeComm(&testcomm);
    }
    while (MTestGetIntercomm(&testcomm, &isLeft, 1)) {
        if (testcomm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(testcomm, &rank);
        MPI_Comm_size(testcomm, &size);
        MPI_Comm_remote_size(testcomm, &rsize);
        cnt = 0;
        for (j = 0; j < ITERS; j++) {
            if (rank == 0) {
                out[j] = 815;
                in[j] = 815;
                sol[j] = 815;
                for (i = 1; i < rsize; i++)
                    MPI_Isend(&out[j], 1, MPI_INT, i, 0, testcomm, &sreq[cnt++]);
                MPI_Comm_idup(testcomm, &newcomm[j], &sreq[cnt++]);
            }
            else {
                out[j] = 0;
                in[j] = 0;
                sol[j] = 815;
                MPI_Comm_idup(testcomm, &newcomm[j], &sreq[cnt++]);
                MPI_Irecv(&in[j], 1, MPI_INT, 0, 0, testcomm, &sreq[cnt++]);
            }
        }
        MPI_Waitall(cnt, sreq, MPI_STATUS_IGNORE);

        for (j = 0; j < ITERS; j++) {
            if (sol[j] != in[j])
                errs++;
            errs += MTestTestComm(newcomm[j]);
            MPI_Comm_free(&newcomm[j]);
        }
        MTestFreeComm(&testcomm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
