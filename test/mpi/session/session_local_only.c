/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "stdio.h"

/*
static char MTEST_Descrip[] = "Test MPI Sessions local-only behavior.
No communication is performed. Intended to run with a minimum of 2 MPI
processes and at least 1 non-MPI process. This verifies the local,
non-collective nature of MPI Sessions initialization and group
construction APIs. Example launch command:

  mpiexec -n 2 ./session_local_only : -n 1 true

The world group size will be 3, but only 2 processes will initialize
and make MPI calls. A correct implementation should not hang.
*/

int main(int argc, char *argv[])
{
    MPI_Session session;
    MPI_Group group;
    int rank, size;

    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &session);

    MPI_Group_from_session_pset(session, "mpi://world", &group);
    MPI_Group_rank(group, &rank);
    MPI_Group_size(group, &size);
    if (size < 3) {
        /* no communicator so directly call errhandler */
        MPI_Session_call_errhandler(session, 1);
    }

    MPI_Group_free(&group);
    MPI_Session_finalize(&session);
    if (rank == 0) {
        printf("No Errors\n");
    }

    return 0;
}
