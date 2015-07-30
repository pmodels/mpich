/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Use of MPI_BOTTOM in communication";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, source, dest, len, ii;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Datatype newtype, oldtype;
    MPI_Aint disp;

    MTest_Init(&argc, &argv);

    MPI_Get_address(&ii, &disp);

    len = 1;
    oldtype = MPI_INT;
    MPI_Type_create_struct(1, &len, &disp, &oldtype, &newtype);
    MPI_Type_commit(&newtype);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    if (size < 2) {
        errs++;
        fprintf(stderr, "This test requires at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    source = 0;
    dest = 1;

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    if (rank == source) {
        ii = 2;
        err = MPI_Send(MPI_BOTTOM, 1, newtype, dest, 0, comm);
        if (err) {
            errs++;
            MTestPrintError(err);
            printf("MPI_Send did not return MPI_SUCCESS\n");
        }
    }
    else if (rank == dest) {
        ii = -1;
        err = MPI_Recv(MPI_BOTTOM, 1, newtype, source, 0, comm, &status);
        if (err) {
            MTestPrintError(err);
            errs++;
            printf("MPI_Recv did not return MPI_SUCCESS\n");
        }
        if (ii != 2) {
            errs++;
            printf("Received %d but expected %d\n", ii, 2);
        }
    }

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);

    MPI_Type_free(&newtype);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
