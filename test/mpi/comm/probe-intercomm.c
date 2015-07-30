/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_Probe() for an intercomm";
*/
#define MAX_DATA_LEN 100

int main(int argc, char *argv[])
{
    int errs = 0, recvlen, isLeft;
    MPI_Status status;
    int rank, size;
    MPI_Comm intercomm;
    char buf[MAX_DATA_LEN];
    const char *test_str = "test";

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (MTestGetIntercomm(&intercomm, &isLeft, 2)) {
        if (intercomm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(intercomm, &rank);

        /* 0 ranks on each side communicate, everyone else does nothing */
        if (rank == 0) {
            if (isLeft) {
                recvlen = -1;
                MPI_Probe(0, 0, intercomm, &status);
                MPI_Get_count(&status, MPI_CHAR, &recvlen);
                if (recvlen != (strlen(test_str) + 1)) {
                    printf(" Error: recvlen (%d) != strlen(\"%s\")+1 (%d)\n", recvlen, test_str,
                           (int) strlen(test_str) + 1);
                    ++errs;
                }
                buf[0] = '\0';
                MPI_Recv(buf, recvlen, MPI_CHAR, 0, 0, intercomm, &status);
                if (strcmp(test_str, buf)) {
                    printf(" Error: strcmp(test_str,buf)!=0\n");
                    ++errs;
                }
            }
            else {
                strncpy(buf, test_str, 5);
                MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, 0, 0, intercomm);
            }
        }
        MTestFreeComm(&intercomm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
