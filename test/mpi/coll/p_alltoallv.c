/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_p_alltoallv
int run(const char *arg);
#endif

/*
  This program tests MPI_Alltoallv_init by having processor each process
  send data to two neighbors only, using counts of 0 for the other processes.
  This idiom is sometimes used for halo exchange operations.

  Because there are separate send and receive types to alltoallv,
  there need to be tests to rearrange data on the fly.  Not done yet.

  Currently, the test uses only MPI_INT; this is adequate for testing systems
  that use point-to-point operations
 */

int run(const char *arg)
{

    MPI_Comm comm;
    int *sbuf, *rbuf;
    int rank, size;
    int *sendcounts, *recvcounts, *rdispls, *sdispls;
    int i, j, *p, errs;
    int left, right, length;
    MPI_Info info;
    MPI_Request req;

    errs = 0;

    MPI_Info_create(&info);

    while (MTestGetIntracommGeneral(&comm, 2, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);

        if (size < 3)
            continue;

        /* Create and load the arguments to alltoallv */
        sendcounts = (int *) malloc(size * sizeof(int));
        recvcounts = (int *) malloc(size * sizeof(int));
        rdispls = (int *) malloc(size * sizeof(int));
        sdispls = (int *) malloc(size * sizeof(int));
        if (!sendcounts || !recvcounts || !rdispls || !sdispls) {
            fprintf(stderr, "Could not allocate arg items!\n");
            MPI_Abort(comm, 1);
        }

        /* Get the neighbors */
        left = (rank - 1 + size) % size;
        right = (rank + 1) % size;

        /* Set the defaults */
        for (i = 0; i < size; i++) {
            sendcounts[i] = 0;
            recvcounts[i] = 0;
            rdispls[i] = 0;
            sdispls[i] = 0;
        }

        for (length = 1; length < 66000; length = length * 2 + 1) {
            /* Get the buffers */
            sbuf = (int *) malloc(2 * length * sizeof(int));
            rbuf = (int *) malloc(2 * length * sizeof(int));
            if (!sbuf || !rbuf) {
                fprintf(stderr, "Could not allocate buffers!\n");
                MPI_Abort(comm, 1);
            }

            /* Load up the buffers */
            for (i = 0; i < length; i++) {
                sbuf[i] = i + 100000 * rank;
                sbuf[i + length] = i + 100000 * rank;
                rbuf[i] = -i;
                rbuf[i + length] = -i - length;
            }
            sendcounts[left] = length;
            sendcounts[right] = length;
            recvcounts[left] = length;
            recvcounts[right] = length;
            rdispls[left] = 0;
            rdispls[right] = length;
            sdispls[left] = 0;
            sdispls[right] = length;

            MPI_Alltoallv_init(sbuf, sendcounts, sdispls, MPI_INT, rbuf, recvcounts, rdispls,
                               MPI_INT, comm, info, &req);
            for (j = 0; j < 10; ++j) {
                MPI_Start(&req);
                MPI_Wait(&req, MPI_STATUS_IGNORE);

                /* Check rbuf */
                p = rbuf;       /* left */

                for (i = 0; i < length; i++) {
                    if (p[i] != i + 100000 * left) {
                        if (errs < 10) {
                            fprintf(stderr, "[%d from %d] got %d expected %d for %dth\n",
                                    rank, left, p[i], i + 100000 * left, i);
                        }
                        errs++;
                    }
                }

                p = rbuf + length;      /* right */
                for (i = 0; i < length; i++) {
                    if (p[i] != i + 100000 * right) {
                        if (errs < 10) {
                            fprintf(stderr, "[%d from %d] got %d expected %d for %dth\n",
                                    rank, right, p[i], i + 100000 * right, i);
                        }
                        errs++;
                    }
                }
            }

            MPI_Request_free(&req);

            free(rbuf);
            free(sbuf);
        }

        free(sdispls);
        free(rdispls);
        free(recvcounts);
        free(sendcounts);
        MTestFreeComm(&comm);
    }

    MPI_Info_free(&info);

    return errs;
}
