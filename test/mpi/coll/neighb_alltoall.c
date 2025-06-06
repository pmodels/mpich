/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_neighb_alltoall
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

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    int sendbuf[2] = { -(wrank + 1), wrank + 1 };
    int recvbuf[2] = { 0xdeadbeef, 0xdeadbeef };

#if defined(TEST_NEIGHB_COLL)
    /* (wrap)--> 0 <--> 1 <--> ... <--> p-1 <--(wrap) */
    MPI_Cart_create(MPI_COMM_WORLD, 1, &wsize, periods, /*reorder= */ 0, &cart);
    sendbuf[0] = -(wrank + 1);
    sendbuf[1] = wrank + 1;

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

    MPI_Comm_free(&cart);
#endif /* defined(TEST_NEIGHB_COLL) */

    return errs;
}
