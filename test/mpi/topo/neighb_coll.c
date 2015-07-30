/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>
#include "mpitest.h"

#if !defined(USE_STRICT_MPI) && defined(MPICH)
#define TEST_NEIGHB_COLL 1
#endif

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;
    int periods[1] = { 0 };
    MPI_Comm cart, dgraph, graph;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

#if defined(TEST_NEIGHB_COLL)
    /* a basic test for the 10 (5 patterns x {blocking,nonblocking}) MPI-3
     * neighborhood collective routines */

    /* (wrap)--> 0 <--> 1 <--> ... <--> p-1 <--(wrap) */
    MPI_Cart_create(MPI_COMM_WORLD, 1, &wsize, periods, /*reorder= */ 0, &cart);

    /* allgather */
    {
        int sendbuf[1] = { wrank };
        int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };

        /* should see one send to each neighbor (rank-1 and rank+1) and one receive
         * each from same */
        MPI_Neighbor_allgather(sendbuf, 1, MPI_INT, recvbuf, 1, MPI_INT, cart);

        if (wrank == 0)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == wrank - 1);

        if (wrank == wsize - 1)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == wrank + 1);
    }

    /* allgatherv */
    {
        int sendbuf[1] = { wrank };
        int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };
        int recvcounts[2] = { 1, 1 };
        int displs[2] = { 1, 0 };

        /* should see one send to each neighbor (rank-1 and rank+1) and one receive
         * each from same, but put them in opposite slots in the buffer */
        MPI_Neighbor_allgatherv(sendbuf, 1, MPI_INT, recvbuf, recvcounts, displs, MPI_INT, cart);

        if (wrank == 0)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == wrank - 1);

        if (wrank == wsize - 1)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == wrank + 1);
    }

    /* alltoall */
    {
        int sendbuf[2] = { -(wrank + 1), wrank + 1 };
        int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };

        /* should see one send to each neighbor (rank-1 and rank+1) and one
         * receive each from same */
        MPI_Neighbor_alltoall(sendbuf, 1, MPI_INT, recvbuf, 1, MPI_INT, cart);

        if (wrank == 0)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == wrank);

        if (wrank == wsize - 1)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == -(wrank + 2));
    }

    /* alltoallv */
    {
        int sendbuf[2] = { -(wrank + 1), wrank + 1 };
        int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };
        int sendcounts[2] = { 1, 1 };
        int recvcounts[2] = { 1, 1 };
        int sdispls[2] = { 0, 1 };
        int rdispls[2] = { 1, 0 };

        /* should see one send to each neighbor (rank-1 and rank+1) and one receive
         * each from same, but put them in opposite slots in the buffer */
        MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, MPI_INT,
                               recvbuf, recvcounts, rdispls, MPI_INT, cart);

        if (wrank == 0)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == wrank);

        if (wrank == wsize - 1)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == -(wrank + 2));
    }

    /* alltoallw */
    {
        int sendbuf[2] = { -(wrank + 1), wrank + 1 };
        int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };
        int sendcounts[2] = { 1, 1 };
        int recvcounts[2] = { 1, 1 };
        MPI_Aint sdispls[2] = { 0, sizeof(int) };
        MPI_Aint rdispls[2] = { sizeof(int), 0 };
        MPI_Datatype sendtypes[2] = { MPI_INT, MPI_INT };
        MPI_Datatype recvtypes[2] = { MPI_INT, MPI_INT };

        /* should see one send to each neighbor (rank-1 and rank+1) and one receive
         * each from same, but put them in opposite slots in the buffer */
        MPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                               recvbuf, recvcounts, rdispls, recvtypes, cart);

        if (wrank == 0)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == wrank);

        if (wrank == wsize - 1)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == -(wrank + 2));
    }


    MPI_Comm_free(&cart);
#endif /* defined(TEST_NEIGHB_COLL) */

    MPI_Reduce((wrank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (wrank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }
    MPI_Finalize();

    return 0;
}
