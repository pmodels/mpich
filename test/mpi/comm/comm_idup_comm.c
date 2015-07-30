/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test tests overlapping of Comm_idups with other comm. generations calls */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

#define NUM_IDUPS 5

int main(int argc, char **argv)
{
    int errs = 0;
    int i;
    int rank, size;
    int *excl;
    int ranges[1][3];
    int isLeft, rleader;
    MPI_Group world_group, high_group, even_group;
    MPI_Comm local_comm, inter_comm, test_comm, outcomm;
    MPI_Comm idupcomms[NUM_IDUPS];
    MPI_Request reqs[NUM_IDUPS];

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Idup MPI_COMM_WORLD multiple times */
    for (i = 0; i < NUM_IDUPS; i++) {
        MPI_Comm_idup(MPI_COMM_WORLD, &idupcomms[i], &reqs[i]);
    }

    /* Overlap pending idups with various comm generation functions */

    /* Comm_dup */
    MPI_Comm_dup(MPI_COMM_WORLD, &outcomm);
    errs += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_split */
    MPI_Comm_split(MPI_COMM_WORLD, rank % 2, size - rank, &outcomm);
    errs += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_create, high half of MPI_COMM_WORLD */
    ranges[0][0] = size / 2;
    ranges[0][1] = size - 1;
    ranges[0][2] = 1;
    MPI_Group_range_incl(world_group, 1, ranges, &high_group);
    MPI_Comm_create(MPI_COMM_WORLD, high_group, &outcomm);
    MPI_Group_free(&high_group);
    errs += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_create_group, even ranks of MPI_COMM_WORLD */
    /* exclude the odd ranks */
    excl = malloc((size / 2) * sizeof(int));
    for (i = 0; i < size / 2; i++)
        excl[i] = (2 * i) + 1;

    MPI_Group_excl(world_group, size / 2, excl, &even_group);
    free(excl);

    if (rank % 2 == 0) {
        MPI_Comm_create_group(MPI_COMM_WORLD, even_group, 0, &outcomm);
    }
    else {
        outcomm = MPI_COMM_NULL;
    }
    MPI_Group_free(&even_group);

    errs += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Intercomm_create & Intercomm_merge */
    MPI_Comm_split(MPI_COMM_WORLD, (rank < size / 2), rank, &local_comm);

    if (rank == 0) {
        rleader = size / 2;
    }
    else if (rank == size / 2) {
        rleader = 0;
    }
    else {
        rleader = -1;
    }
    isLeft = rank < size / 2;

    MPI_Intercomm_create(local_comm, 0, MPI_COMM_WORLD, rleader, 99, &inter_comm);
    MPI_Intercomm_merge(inter_comm, isLeft, &outcomm);
    MPI_Comm_free(&local_comm);

    errs += MTestTestComm(inter_comm);
    MTestFreeComm(&inter_comm);

    errs += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    MPI_Waitall(NUM_IDUPS, reqs, MPI_STATUSES_IGNORE);
    for (i = 0; i < NUM_IDUPS; i++) {
        errs += MTestTestComm(idupcomms[i]);
        MPI_Comm_free(&idupcomms[i]);
    }

    MPI_Group_free(&world_group);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
