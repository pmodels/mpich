/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "stdio.h"

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
