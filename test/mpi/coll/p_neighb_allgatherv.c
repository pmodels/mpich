/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_p_neighb_allgatherv
int run(const char *arg);
#endif

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

int run(const char *arg)
{
    int errs = 0;
    int wrank, wsize;
    int periods[1] = { 0 };
    MPI_Comm cart;
    int sendbuf[1];
    int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };
    int recvcounts[2] = { 1, 1 };
    int displs[2] = { 1, 0 };
    MPI_Info info;
    MPI_Request req;


    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

#if defined(TEST_NEIGHB_COLL)
    /* (wrap)--> 0 <--> 1 <--> ... <--> p-1 <--(wrap) */
    MPI_Cart_create(MPI_COMM_WORLD, 1, &wsize, periods, /*reorder= */ 0, &cart);
    sendbuf[0] = wrank;

    /* should see one send to each neighbor (rank-1 and rank+1) and one receive
     * each from same, but put them in opposite slots in the buffer */
    MPI_Info_create(&info);
    MPI_Neighbor_allgatherv_init(sendbuf, 1, MPI_INT, recvbuf, recvcounts, displs, MPI_INT, cart,
                                 info, &req);
    for (int i = 0; i < 10; ++i) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        if (wrank == 0)
            check(recvbuf[1] == 0xdeadbeef);
        else
            check(recvbuf[1] == wrank - 1);

        if (wrank == wsize - 1)
            check(recvbuf[0] == 0xdeadbeef);
        else
            check(recvbuf[0] == wrank + 1);
    }
    MPI_Request_free(&req);
    MPI_Info_free(&info);

    MPI_Comm_free(&cart);
#endif /* defined(TEST_NEIGHB_COLL) */

    return errs;
}
