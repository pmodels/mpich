/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  This program tests MPI_Alltoallw by having processor i send different
  amounts of data to each processor.  This is just the MPI_Alltoallv test,
  but with displacements in bytes rather than units of the datatype.

  Because there are separate send and receive types to alltoallw,
  there need to be tests to rearrange data on the fly.  Not done yet.

  The first test sends i items to processor i from all processors.

  Currently, the test uses only MPI_INT; this is adequate for testing systems
  that use point-to-point operations
 */

static int test_noinplace(MPI_Comm comm);
static int test_inplace(MPI_Comm comm, bool sametype);

int main(int argc, char **argv)
{
    int errs = 0;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);
    while (MTestGetIntracommGeneral(&comm, 2, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        errs += test_noinplace(comm);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
        errs += test_inplace(comm, true);
        errs += test_inplace(comm, false);
#endif
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

static int test_noinplace(MPI_Comm comm)
{
    int errs = 0;
    int *sbuf, *rbuf;
    int *sendcounts, *recvcounts, *rdispls, *sdispls;
    MPI_Datatype *sendtypes, *recvtypes;

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    /* Create the buffer */
    sbuf = (int *) malloc(size * size * sizeof(int));
    rbuf = (int *) malloc(size * size * sizeof(int));
    if (!sbuf || !rbuf) {
        fprintf(stderr, "Could not allocated buffers!\n");
        MPI_Abort(comm, 1);
    }

    /* Load up the buffers */
    for (int i = 0; i < size * size; i++) {
        sbuf[i] = i + 100 * rank;
        rbuf[i] = -i;
    }

    /* Create and load the arguments to alltoallv */
    sendcounts = (int *) malloc(size * sizeof(int));
    recvcounts = (int *) malloc(size * sizeof(int));
    rdispls = (int *) malloc(size * sizeof(int));
    sdispls = (int *) malloc(size * sizeof(int));
    sendtypes = (MPI_Datatype *) malloc(size * sizeof(MPI_Datatype));
    recvtypes = (MPI_Datatype *) malloc(size * sizeof(MPI_Datatype));
    if (!sendcounts || !recvcounts || !rdispls || !sdispls || !sendtypes || !recvtypes) {
        fprintf(stderr, "Could not allocate arg items!\n");
        MPI_Abort(comm, 1);
    }
    /* Note that process 0 sends no data (sendcounts[0] = 0) */
    for (int i = 0; i < size; i++) {
        sendcounts[i] = i;
        recvcounts[i] = rank;
        rdispls[i] = i * rank * sizeof(int);
        sdispls[i] = (((i + 1) * (i)) / 2) * sizeof(int);
        sendtypes[i] = recvtypes[i] = MPI_INT;
    }
    MPI_Alltoallw(sbuf, sendcounts, sdispls, sendtypes, rbuf, recvcounts, rdispls, recvtypes, comm);

    /* Check rbuf */
    for (int i = 0; i < size; i++) {
        int *p = rbuf + rdispls[i] / sizeof(int);
        for (int j = 0; j < rank; j++) {
            if (p[j] != i * 100 + (rank * (rank + 1)) / 2 + j) {
                fprintf(stderr, "[%d] got %d expected %d for %dth\n",
                        rank, p[j], (i * (i + 1)) / 2 + j, j);
                errs++;
            }
        }
    }

    free(sendtypes);
    free(sdispls);
    free(sendcounts);
    free(sbuf);
    free(recvtypes);
    free(rdispls);
    free(recvcounts);
    free(rbuf);

    return errs;
}

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
static int test_inplace(MPI_Comm comm, bool sametype)
{
    int errs = 0;
    int *rbuf;
    int *recvcounts, *rdispls;
    MPI_Datatype *recvtypes;

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    /* Create the buffer */
    rbuf = (int *) malloc(size * (2 * size) * sizeof(int));
    if (!rbuf) {
        fprintf(stderr, "Could not reallocate rbuf!\n");
        MPI_Abort(comm, 1);
    }

    /* Load up the buffers */
    memset(rbuf, -1, size * (2 * size) * sizeof(int));
    for (int i = 0; i < size; i++) {
        int *p = rbuf + i * (2 * size);
        for (int j = 0; j < i + rank; ++j) {
            p[j] = 100 * rank + 10 * i + j;
        }
    }

    /* Create and load the arguments to alltoallv */
    recvcounts = (int *) malloc(size * sizeof(int));
    rdispls = (int *) malloc(size * sizeof(int));
    recvtypes = (MPI_Datatype *) malloc(size * sizeof(MPI_Datatype));
    if (!recvcounts || !rdispls || !recvtypes) {
        fprintf(stderr, "Could not allocate arg items!\n");
        MPI_Abort(comm, 1);
    }

    for (int i = 0; i < size; i++) {
        /* alltoallw displs are in bytes, not in type extents */
        rdispls[i] = i * (2 * size) * sizeof(int);
        if (sametype) {
            recvtypes[i] = MPI_INT;
            recvcounts[i] = i + rank;
        } else {
            MPI_Type_contiguous(i + rank, MPI_INT, &recvtypes[i]);
            MPI_Type_commit(&recvtypes[i]);
            recvcounts[i] = 1;
        }
    }

    MPI_Alltoallw(MPI_IN_PLACE, NULL, NULL, NULL, rbuf, recvcounts, rdispls, recvtypes, comm);

    if (!sametype) {
        for (int i = 0; i < size; i++) {
            MPI_Type_free(&recvtypes[i]);
        }
    }

    /* Check rbuf */
    for (int i = 0; i < size; i++) {
        int *p = rbuf + (rdispls[i] / sizeof(int));
        for (int j = 0; j < i + rank; j++) {
            int expected = 100 * i + 10 * rank + j;
            if (p[j] != expected) {
                fprintf(stderr, "[%d] got %d expected %d for block=%d, element=%dth\n",
                        rank, p[j], expected, i, j);
                ++errs;
            }
        }
    }

    free(recvtypes);
    free(rdispls);
    free(recvcounts);
    free(rbuf);

    return errs;
}
#endif
