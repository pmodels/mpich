/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Based on a test case contributed by Michael Hofmann.
 *
 * This test makes sure that zero counts with non-zero-sized types on the
 * send (recv) side match and don't cause a problem with non-zero counts and
 * zero-sized types on the recv (send) side when using MPI_Alltoallw and
 * MPI_Alltoallv.  */

/* TODO test intercommunicators as well */


#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "mpitest.h"

int main(int argc, char *argv[])
{
    int sendbuf, recvbuf;
    int *sendcounts;
    int *recvcounts;
    int *sdispls;
    int *rdispls;
    MPI_Datatype sendtype;
    MPI_Datatype *sendtypes;
    MPI_Datatype *recvtypes;
    int rank = -1;
    int size = -1;
    int i;


    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    sendtypes = malloc(size * sizeof(MPI_Datatype));
    recvtypes = malloc(size * sizeof(MPI_Datatype));
    sendcounts = malloc(size * sizeof(int));
    recvcounts = malloc(size * sizeof(int));
    sdispls = malloc(size * sizeof(int));
    rdispls = malloc(size * sizeof(int));
    if (!sendtypes  || !recvtypes ||
        !sendcounts || !recvcounts ||
        !sdispls    || !rdispls)
    {
        printf("error, unable to allocate memory\n");
        goto fn_exit;
    }

    MPI_Type_contiguous(0, MPI_INT, &sendtype);
    MPI_Type_commit(&sendtype);

    for (i = 0; i < size; ++i) {
        sendtypes[i] = sendtype;
        sendcounts[i] = 1;
        sdispls[i] = 0;

        recvtypes[i] = MPI_INT;
        recvcounts[i] = 0;
        rdispls[i] = 0;
    }


    /* try zero-counts on both the send and recv side in case only one direction is broken for some reason */
    MPI_Alltoallw(&sendbuf, sendcounts, sdispls, sendtypes, &recvbuf, recvcounts, rdispls, recvtypes, MPI_COMM_WORLD);
    MPI_Alltoallw(&sendbuf, recvcounts, rdispls, recvtypes, &recvbuf, sendcounts, sdispls, sendtypes, MPI_COMM_WORLD);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* pass MPI_IN_PLACE and different but compatible types rank is even/odd */
    if (rank % 2)
        MPI_Alltoallw(MPI_IN_PLACE, NULL, NULL, NULL, &recvbuf, recvcounts, rdispls, recvtypes, MPI_COMM_WORLD);
    else
        MPI_Alltoallw(MPI_IN_PLACE, NULL, NULL, NULL, &recvbuf, sendcounts, sdispls, sendtypes, MPI_COMM_WORLD);
#endif

    /* now the same for Alltoallv instead of Alltoallw */
    MPI_Alltoallv(&sendbuf, sendcounts, sdispls, sendtypes[0], &recvbuf, recvcounts, rdispls, recvtypes[0], MPI_COMM_WORLD);
    MPI_Alltoallv(&sendbuf, recvcounts, rdispls, recvtypes[0], &recvbuf, sendcounts, sdispls, sendtypes[0], MPI_COMM_WORLD);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    if (rank % 2)
        MPI_Alltoallv(MPI_IN_PLACE, NULL, NULL, MPI_DATATYPE_NULL, &recvbuf, recvcounts, rdispls, recvtypes[0], MPI_COMM_WORLD);
    else
        MPI_Alltoallv(MPI_IN_PLACE, NULL, NULL, MPI_DATATYPE_NULL, &recvbuf, sendcounts, sdispls, sendtypes[0], MPI_COMM_WORLD);
#endif

    MPI_Type_free(&sendtype);

    if (rank == 0)
        printf(" No Errors\n");

fn_exit:
    if (rdispls)    free(rdispls);
    if (sdispls)    free(sdispls);
    if (recvcounts) free(recvcounts);
    if (sendcounts) free(sendcounts);
    if (recvtypes)  free(recvtypes);
    if (sendtypes)  free(sendtypes);

    MPI_Finalize();

    return 0;
}

