/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This program tests that MPI_Comm_create applies to intercommunicators;
 * this is an extension added in MPI-2
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int size, isLeft, wrank;
    MPI_Comm intercomm, newcomm;
    MPI_Group oldgroup, newgroup;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 4) {
        printf("This test requires at least 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    while (MTestGetIntercomm(&intercomm, &isLeft, 2)) {
        int ranks[10], nranks, result;

        if (intercomm == MPI_COMM_NULL)
            continue;

        MPI_Comm_group(intercomm, &oldgroup);
        ranks[0] = 0;
        nranks = 1;
        MTestPrintfMsg(1, "Creating a new intercomm 0-0\n");
        MPI_Group_incl(oldgroup, nranks, ranks, &newgroup);
        MPI_Comm_create(intercomm, newgroup, &newcomm);

        /* Make sure that the new communicator has the appropriate pieces */
        if (newcomm != MPI_COMM_NULL) {
            int new_rsize, new_size, flag, commok = 1;

            MPI_Comm_set_name(newcomm, (char *) "Single rank in each group");
            MPI_Comm_test_inter(intercomm, &flag);
            if (!flag) {
                errs++;
                printf("[%d] Output communicator is not an intercomm\n", wrank);
                commok = 0;
            }

            MPI_Comm_remote_size(newcomm, &new_rsize);
            MPI_Comm_size(newcomm, &new_size);
            /* The new communicator has 1 process in each group */
            if (new_rsize != 1) {
                errs++;
                printf("[%d] Remote size is %d, should be one\n", wrank, new_rsize);
                commok = 0;
            }
            if (new_size != 1) {
                errs++;
                printf("[%d] Local size is %d, should be one\n", wrank, new_size);
                commok = 0;
            }
            /* ... more to do */
            if (commok) {
                errs += MTestTestComm(newcomm);
            }
        }
        MPI_Group_free(&newgroup);
        if (newcomm != MPI_COMM_NULL) {
            MPI_Comm_free(&newcomm);
        }

        /* Now, do a sort of dup, using the original group */
        MTestPrintfMsg(1, "Creating a new intercomm (manual dup)\n");
        MPI_Comm_create(intercomm, oldgroup, &newcomm);
        MPI_Comm_set_name(newcomm, (char *) "Dup of original");
        MTestPrintfMsg(1, "Creating a new intercomm (manual dup (done))\n");

        MPI_Comm_compare(intercomm, newcomm, &result);
        MTestPrintfMsg(1, "Result of comm/intercomm compare is %d\n", result);
        if (result != MPI_CONGRUENT) {
            const char *rname = 0;
            errs++;
            switch (result) {
            case MPI_IDENT:
                rname = "IDENT";
                break;
            case MPI_CONGRUENT:
                rname = "CONGRUENT";
                break;
            case MPI_SIMILAR:
                rname = "SIMILAR";
                break;
            case MPI_UNEQUAL:
                rname = "UNEQUAL";
                break;
                printf("[%d] Expected MPI_CONGRUENT but saw %d (%s)", wrank, result, rname);
                fflush(stdout);
            }
        }
        else {
            /* Try to communication between each member of intercomm */
            errs += MTestTestComm(newcomm);
        }

        if (newcomm != MPI_COMM_NULL) {
            MPI_Comm_free(&newcomm);
        }
        /* test that an empty group in either side of the intercomm results in
         * MPI_COMM_NULL for all members of the comm */
        if (isLeft) {
            /* left side reuses oldgroup, our local group in intercomm */
            MPI_Comm_create(intercomm, oldgroup, &newcomm);
        }
        else {
            /* right side passes MPI_GROUP_EMPTY */
            MPI_Comm_create(intercomm, MPI_GROUP_EMPTY, &newcomm);
        }
        if (newcomm != MPI_COMM_NULL) {
            printf("[%d] expected MPI_COMM_NULL, but got a different communicator\n", wrank);
            fflush(stdout);
            errs++;
        }

        if (newcomm != MPI_COMM_NULL) {
            MPI_Comm_free(&newcomm);
        }
        MPI_Group_free(&oldgroup);
        MPI_Comm_free(&intercomm);
    }

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
