/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_exscan2
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test MPI_Exscan (simple test)";
*/

int run(const char *arg)
{
    int errs = 0;
    int rank, size;
    int sendbuf[1], recvbuf[1];
    MPI_Comm comm;

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    sendbuf[0] = rank;
    recvbuf[0] = -2;

    MPI_Exscan(sendbuf, recvbuf, 1, MPI_INT, MPI_SUM, comm);

    /* Check the results.  rank 0 has no data.  Input is
     * 0  1  2  3  4  5  6  7  8 ...
     * Output is
     * -  0  1  3  6 10 15 21 28 36
     * (scan, not counting the contribution from the calling process)
     */
    if (rank > 0) {
        int result = (((rank) * (rank - 1)) / 2);
        /* printf("%d: %d\n", rank, result); */
        if (recvbuf[0] != result) {
            errs++;
            fprintf(stderr, "Error in recvbuf = %d on %d, expected %d\n", recvbuf[0], rank, result);
        }
    } else if (recvbuf[0] != -2) {
        errs++;
        fprintf(stderr, "Error in recvbuf on zero, is %d\n", recvbuf[0]);
    }

    return errs;
}
