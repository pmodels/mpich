/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Neighborhood alltoall on communicators whose neighbor lists contain the same
 * process more than once.
 *
 * Cartesian part: a 3-D grid with two periodic dimensions of size 1, so that
 * four of the six blocks are exchanged with self and the two remaining ones
 * with a single peer.  MPI-4.1 Example 8.10 fixes the matching: the block sent
 * in the negative direction of dimension d is received into block 2*d+1 of the
 * neighbor, and vice versa, so block s of the sender lands in block s^1 of the
 * receiver.  With sendbuf[s] = rank * NSLOT + s the expected content of the
 * receive buffer is therefore known in closed form.
 *
 * ring_neighbor_alltoall covers a single degenerate dimension; this test covers
 * more than one, where the duplicated edges of different dimensions all refer
 * to the same process.
 *
 * Distributed graph part: the same neighborhood built with
 * MPI_Dist_graph_create_adjacent.  There the neighbor sequence is user defined
 * and MPI-4.1 Section 8.6 matches the k-th edge to a process with the k-th edge
 * from it, which the point-to-point reference below reproduces with one tag per
 * multiplicity index.
 */

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"

#define NDIMS 3
#define NSLOT (2 * NDIMS)

static int rank;

static int check(const char *what, const int *got, const int *exp)
{
    int errs = 0;
    for (int s = 0; s < NSLOT; s++) {
        if (got[s] != exp[s]) {
            if (errs < 10) {
                printf("%s: rank %d block %d is %d, expected %d\n", what, rank, s, got[s], exp[s]);
            }
            errs++;
        }
    }
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int size;
    int nbrs[NSLOT];
    int sbuf[NSLOT], rbuf[NSLOT], expected[NSLOT];
    int counts[NSLOT], displs[NSLOT];
    MPI_Aint wdispls[NSLOT];
    MPI_Datatype types[NSLOT];
    MPI_Comm cart, dgraph;
    MPI_Request req;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int dims[NDIMS] = { 1, 1, size };
    int periods[NDIMS] = { 1, 1, 1 };
    MPI_Cart_create(MPI_COMM_WORLD, NDIMS, dims, periods, /*reorder= */ 0, &cart);

    for (int d = 0; d < NDIMS; d++) {
        MPI_Cart_shift(cart, d, 1, &nbrs[2 * d], &nbrs[2 * d + 1]);
    }

    for (int s = 0; s < NSLOT; s++) {
        sbuf[s] = rank * NSLOT + s;
        counts[s] = 1;
        displs[s] = s;
        wdispls[s] = (MPI_Aint) (s * sizeof(int));
        types[s] = MPI_INT;
        /* MPI-4.1 Example 8.10: block s of neighbor nbrs[s] arrives here in
         * block s ^ 1 -- equivalently, our block s holds their block s ^ 1. */
        expected[s] = nbrs[s] * NSLOT + (s ^ 1);
    }

#define RUN_CART(name_, call_)                                          \
    do {                                                                \
        memset(rbuf, 0, sizeof(rbuf));                                  \
        call_;                                                          \
        errs += check(name_, rbuf, expected);                           \
    } while (0)

    RUN_CART("MPI_Neighbor_alltoall",
             MPI_Neighbor_alltoall(sbuf, 1, MPI_INT, rbuf, 1, MPI_INT, cart));
    RUN_CART("MPI_Neighbor_alltoallv",
             MPI_Neighbor_alltoallv(sbuf, counts, displs, MPI_INT,
                                    rbuf, counts, displs, MPI_INT, cart));
    RUN_CART("MPI_Neighbor_alltoallw",
             MPI_Neighbor_alltoallw(sbuf, counts, wdispls, types,
                                    rbuf, counts, wdispls, types, cart));

    MPI_Ineighbor_alltoall(sbuf, 1, MPI_INT, rbuf, 1, MPI_INT, cart, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    errs += check("MPI_Ineighbor_alltoall", rbuf, expected);

    MPI_Ineighbor_alltoallv(sbuf, counts, displs, MPI_INT,
                            rbuf, counts, displs, MPI_INT, cart, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    errs += check("MPI_Ineighbor_alltoallv", rbuf, expected);

    MPI_Ineighbor_alltoallw(sbuf, counts, wdispls, types, rbuf, counts, wdispls, types, cart, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    errs += check("MPI_Ineighbor_alltoallw", rbuf, expected);

    /* Same neighborhood as a distributed graph.  reorder is off above, so the
     * ranks in nbrs[] are valid in both communicators. */
    MPI_Dist_graph_create_adjacent(cart, NSLOT, nbrs, MPI_UNWEIGHTED,
                                   NSLOT, nbrs, MPI_UNWEIGHTED,
                                   MPI_INFO_NULL, /*reorder= */ 0, &dgraph);

    /* MPI-4.1 Section 8.6: the k-th edge to a process matches the k-th edge
     * from it.  One tag per multiplicity index expresses exactly that. */
    MPI_Request reqs[2 * NSLOT];
    for (int s = 0; s < NSLOT; s++) {
        int tag = 0;
        for (int m = 0; m < s; m++) {
            if (nbrs[m] == nbrs[s]) {
                tag++;
            }
        }
        MPI_Irecv(&expected[s], 1, MPI_INT, nbrs[s], tag, cart, &reqs[s]);
        MPI_Isend(&sbuf[s], 1, MPI_INT, nbrs[s], tag, cart, &reqs[NSLOT + s]);
    }
    MPI_Waitall(2 * NSLOT, reqs, MPI_STATUSES_IGNORE);

    memset(rbuf, 0, sizeof(rbuf));
    MPI_Neighbor_alltoallv(sbuf, counts, displs, MPI_INT, rbuf, counts, displs, MPI_INT, dgraph);
    errs += check("MPI_Neighbor_alltoallv (dist graph)", rbuf, expected);

    memset(rbuf, 0, sizeof(rbuf));
    MPI_Ineighbor_alltoallv(sbuf, counts, displs, MPI_INT,
                            rbuf, counts, displs, MPI_INT, dgraph, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    errs += check("MPI_Ineighbor_alltoallv (dist graph)", rbuf, expected);

    MPI_Comm_free(&dgraph);
    MPI_Comm_free(&cart);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
