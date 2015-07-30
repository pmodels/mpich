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
 * This program tests that MPI_Comm_split applies to intercommunicators;
 * this is an extension added in MPI-2
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int size, isLeft;
    MPI_Comm intercomm, newcomm;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 4) {
        printf("This test requires at least 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (MTestGetIntercomm(&intercomm, &isLeft, 2)) {
        int key, color;

        if (intercomm == MPI_COMM_NULL)
            continue;

        /* Split this intercomm.  The new intercomms contain the
         * processes that had odd (resp even) rank in their local group
         * in the original intercomm */
        MTestPrintfMsg(1, "Created intercomm %s\n", MTestGetIntercommName());
        MPI_Comm_rank(intercomm, &key);
        color = (key % 2);
        MPI_Comm_split(intercomm, color, key, &newcomm);
        /* Make sure that the new communicator has the appropriate pieces */
        if (newcomm != MPI_COMM_NULL) {
            int orig_rsize, orig_size, new_rsize, new_size;
            int predicted_size, flag, commok = 1;

            MPI_Comm_test_inter(intercomm, &flag);
            if (!flag) {
                errs++;
                printf("Output communicator is not an intercomm\n");
                commok = 0;
            }

            MPI_Comm_remote_size(intercomm, &orig_rsize);
            MPI_Comm_remote_size(newcomm, &new_rsize);
            MPI_Comm_size(intercomm, &orig_size);
            MPI_Comm_size(newcomm, &new_size);
            /* The local size is 1/2 the original size, +1 if the
             * size was odd and the color was even.  More precisely,
             * let n be the orig_size.  Then
             * color 0     color 1
             * orig size even    n/2         n/2
             * orig size odd     (n+1)/2     n/2
             *
             * However, since these are integer valued, if n is even,
             * then (n+1)/2 = n/2, so this table is much simpler:
             * color 0     color 1
             * orig size even    (n+1)/2     n/2
             * orig size odd     (n+1)/2     n/2
             *
             */
            predicted_size = (orig_size + !color) / 2;
            if (predicted_size != new_size) {
                errs++;
                printf("Predicted size = %d but found %d for %s (%d,%d)\n",
                       predicted_size, new_size, MTestGetIntercommName(), orig_size, orig_rsize);
                commok = 0;
            }
            predicted_size = (orig_rsize + !color) / 2;
            if (predicted_size != new_rsize) {
                errs++;
                printf("Predicted remote size = %d but found %d for %s (%d,%d)\n",
                       predicted_size, new_rsize, MTestGetIntercommName(), orig_size, orig_rsize);
                commok = 0;
            }
            /* ... more to do */
            if (commok) {
                errs += MTestTestComm(newcomm);
            }
        }
        else {
            int orig_rsize;
            /* If the newcomm is null, then this means that remote group
             * for this color is of size zero (since all processes in this
             * test have been given colors other than MPI_UNDEFINED).
             * Confirm that here */
            /* FIXME: ToDo */
            MPI_Comm_remote_size(intercomm, &orig_rsize);
            if (orig_rsize == 1) {
                if (color == 0) {
                    errs++;
                    printf("Returned null intercomm when non-null expected\n");
                }
            }
        }
        if (newcomm != MPI_COMM_NULL)
            MPI_Comm_free(&newcomm);
        MPI_Comm_free(&intercomm);
    }
    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
