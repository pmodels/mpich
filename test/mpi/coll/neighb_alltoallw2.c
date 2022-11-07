/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test MPI_Neighbor_alltoallw with different indegree and outdegree */

/* Reference issue #6233. Credit: tjahns */

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 3) {
        printf("This test require exactly 3 processes\n");
        goto fn_exit;
    }

    /* build dist graph where rank 0 sends to rank 1 and 2 */
    static const int indegree[3] = { 0, 1, 1 };
    static const int sources[3][1] = { {-1}, {0}, {0} };
    static const int outdegree[3] = { 2, 0, 0 };
    static const int destinations[3][2] = { {1, 2}, {-1, -1}, {-1, -1} };
    enum { reorder = 0 };
    MPI_Comm neigh_comm;

    MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,
                                   indegree[rank], sources[rank], MPI_UNWEIGHTED,
                                   outdegree[rank], destinations[rank], MPI_UNWEIGHTED,
                                   MPI_INFO_NULL, reorder, &neigh_comm);

    /* send single integer from rank 0 to rank 1 and 2 */
    static const int send_buf[3][1] = { 0, -1, -2 };
    static const int sendcounts[3][2] = { {1, 1}, {-1, -1}, {-1, -1} };
    static const MPI_Aint sdispls[3][2] = { {0, 0}, {-1, -1}, {-1, -1} };
    static const MPI_Datatype sendtypes[3][2] = {
        {MPI_INT, MPI_INT},
        {MPI_DATATYPE_NULL, MPI_DATATYPE_NULL},
        {MPI_DATATYPE_NULL, MPI_DATATYPE_NULL}
    };
    int recv_buf[3][1] = { {-3}, {-4}, {-5} };
    static const int recvcounts[3][1] = { {-1}, {1}, {1} };
    static const MPI_Aint rdispls[3][1] = { {-1}, {0}, {0} };
    static const MPI_Datatype recvtypes[3][2] = { {MPI_DATATYPE_NULL}, {MPI_INT}, {MPI_INT} };

    MPI_Neighbor_alltoallw(send_buf[rank], sendcounts[rank], sdispls[rank], sendtypes[rank],
                           recv_buf[rank], recvcounts[rank], rdispls[rank], recvtypes[rank],
                           neigh_comm);

    /* check receive buffer */
    static const int ref_recv_buf[3][1] = { {-3}, {0}, {0} };

    if (recv_buf[rank][0] != ref_recv_buf[rank][0]) {
        errs++;
        printf("rank: %d, expected: %d, actual value: %d\n",
               rank, ref_recv_buf[rank][0], recv_buf[rank][0]);
    }

    MPI_Comm_free(&neigh_comm);

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
