/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpicolltest.h"

#ifdef MULTI_TESTS
#define run coll_icalltoallv
int run(const char *arg);
#endif

/*
  This program tests MPI_Alltoallv by having processor i send different
  amounts of data to each processor.

  Because there are separate send and receive types to alltoallv,
  there need to be tests to rearrange data on the fly.  Not done yet.

  The first test sends i items to processor i from all processors.

  Currently, the test uses only MPI_INT; this is adequate for testing systems
  that use point-to-point operations
 */

int run(const char *arg)
{
    MPI_Comm comm;
    int *sbuf, *rbuf;
    int rank, size, lsize, asize;
    int *sendcounts, *recvcounts, *rdispls, *sdispls;
    int i, j, *p, errs;
    int leftGroup;

    int is_blocking = 1;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    if (MTestArgListGetInt_with_default(head, "nonblocking", 0)) {
        is_blocking = 0;
    }
    MTestArgListDestroy(head);

    errs = 0;

    while (MTestGetIntercomm(&comm, &leftGroup, 4)) {
        if (comm == MPI_COMM_NULL)
            continue;

        /* Create the buffer */
        MPI_Comm_size(comm, &lsize);
        MPI_Comm_remote_size(comm, &size);
        asize = (lsize > size) ? lsize : size;
        MPI_Comm_rank(comm, &rank);
        sbuf = (int *) malloc(size * size * sizeof(int));
        rbuf = (int *) malloc(asize * asize * sizeof(int));
        if (!sbuf || !rbuf) {
            fprintf(stderr, "Could not allocated buffers!\n");
            MPI_Abort(comm, 1);
        }

        /* Load up the buffers */
        for (i = 0; i < size * size; i++) {
            sbuf[i] = i + 100 * rank;
            rbuf[i] = -i;
        }

        /* Create and load the arguments to alltoallv */
        sendcounts = (int *) malloc(size * sizeof(int));
        recvcounts = (int *) malloc(size * sizeof(int));
        rdispls = (int *) malloc(size * sizeof(int));
        sdispls = (int *) malloc(size * sizeof(int));
        if (!sendcounts || !recvcounts || !rdispls || !sdispls) {
            fprintf(stderr, "Could not allocate arg items!\n");
            MPI_Abort(comm, 1);
        }
        for (i = 0; i < size; i++) {
            sendcounts[i] = i;
            sdispls[i] = (i * (i + 1)) / 2;
            recvcounts[i] = rank;
            rdispls[i] = i * rank;
        }
        MTest_Alltoallv(is_blocking, sbuf, sendcounts, sdispls, MPI_INT,
                        rbuf, recvcounts, rdispls, MPI_INT, comm);

        /* Check rbuf */
        for (i = 0; i < size; i++) {
            p = rbuf + rdispls[i];
            for (j = 0; j < rank; j++) {
                if (p[j] != i * 100 + (rank * (rank + 1)) / 2 + j) {
                    fprintf(stderr, "[%d] got %d expected %d for %dth\n",
                            rank, p[j], (i * (i + 1)) / 2 + j, j);
                    errs++;
                }
            }
        }

        free(sdispls);
        free(rdispls);
        free(recvcounts);
        free(sendcounts);
        free(rbuf);
        free(sbuf);
        MTestFreeComm(&comm);
    }

    return errs;
}
