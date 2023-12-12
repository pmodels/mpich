/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_recv_any
int run(const char *arg);
#endif

#define BUFSIZE 4
#define ITER 10

/* This program tests MPI_Recv or MPI_Irecv + MPI_Wait with MPI_ANY_SOURCE by
 * checking both the value of receive buffer and status.MPI_SOURCE in every
 * iteration.*/

#define ERR_REPORT_EXIT(str,...) do {                                \
        fprintf(stderr, str, ## __VA_ARGS__);fflush(stderr);    \
        MPI_Abort(MPI_COMM_WORLD, 1);                           \
    } while (0);

int run(const char *arg)
{
    int test_nb = 0;
    int rank = 0, nprocs = 0;
    int i = 0, x = 0, dst = 0, src = 0, tag = 0;
    MPI_Status stat;
    MPI_Request req;
    int sbuf[BUFSIZE], rbuf[BUFSIZE];

    MTestArgList *head = MTestArgListCreate_arg(arg);
    test_nb = MTestArgListGetInt_with_default(head, "nb", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* initialize buffers */
    for (i = 0; i < BUFSIZE; i++) {
        sbuf[i] = i + 1;
        rbuf[i] = 0;
    }

    dst = 0;
    src = 1;
    for (x = 0; x < ITER; x++) {
        tag = x;
        if (rank == dst) {
            if (test_nb) {
                MPI_Irecv(rbuf, sizeof(int) * BUFSIZE, MPI_CHAR, MPI_ANY_SOURCE, tag,
                          MPI_COMM_WORLD, &req);
                MPI_Wait(&req, &stat);
            } else {
                MPI_Recv(rbuf, sizeof(int) * BUFSIZE, MPI_CHAR,
                         MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &stat);
            }
            if (stat.MPI_SOURCE != src) {
                ERR_REPORT_EXIT("[%d] Error: iter %d, stat.MPI_SOURCE=%d, expected %d\n",
                                rank, x, stat.MPI_SOURCE, src);
            }

            for (i = 0; i < BUFSIZE; i++) {
                if (rbuf[i] != i + 1) {
                    ERR_REPORT_EXIT("[%d] Error: iter %d, got rbuf[%d]=%d, expected %d\n",
                                    rank, x, i, rbuf[i], i + 1);
                }
            }
        } else if (rank == src) {
            MPI_Send(sbuf, sizeof(int) * BUFSIZE, MPI_CHAR, dst, tag, MPI_COMM_WORLD);
        }
    }

    return 0;
}
