/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpicolltest.h"

/*
static char MTEST_Descrip[] = "Simple intercomm barrier test";
*/

/* This only checks that the Barrier operation accepts intercommunicators.
   It does not check for the semantics of a intercomm barrier (all processes
   in the local group can exit when (but not before) all processes in the
   remote group enter the barrier */
int main(int argc, char *argv[])
{
    int errs = 0, err;
    int leftGroup;
    MPI_Comm comm;

    int is_blocking = 1;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    if (MTestArgListGetInt_with_default(head, "nonblocking", 0)) {
        is_blocking = 0;
    }
    MTestArgListDestroy(head);

    MTest_Init(&argc, &argv);

    /* Get an intercommunicator */
    while (MTestGetIntercomm(&comm, &leftGroup, 4)) {
        if (comm == MPI_COMM_NULL)
            continue;

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
        if (leftGroup) {
            err = MTest_Barrier(is_blocking, comm);
            if (err) {
                errs++;
                MTestPrintError(err);
            }
        } else {
            /* In the right group */
            err = MTest_Barrier(is_blocking, comm);
            if (err) {
                errs++;
                MTestPrintError(err);
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
