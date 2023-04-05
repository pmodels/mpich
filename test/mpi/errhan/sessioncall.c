/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Test for MPI_Session_call_errhandler() with user-defined error handler */

static int calls = 0;
static int errs = 0;
static MPI_Session mysession;

void eh(MPI_Session * session, int *err, ...)
{
    if (*err != MPI_ERR_OTHER) {
        errs++;
        fprintf(stderr, "Unexpected error code\n");
    }
    if (*session != mysession) {
        errs++;
        fprintf(stderr, "Unexpected session\n");
    }
    calls++;
    return;
}

int main(int argc, char *argv[])
{
    MPI_Session session;
    MPI_Errhandler newerr;

    MPI_Session_create_errhandler(eh, &newerr);

    MPI_Session_init(MPI_INFO_NULL, newerr, &session);

    mysession = session;

    MPI_Session_set_errhandler(session, newerr);
    MPI_Session_call_errhandler(session, MPI_ERR_OTHER);
    MPI_Errhandler_free(&newerr);
    if (calls != 1) {
        errs++;
        fprintf(stderr, "Error handler called %d times (expected exactly 1 call)\n", calls);
    }

    MPI_Session_finalize(&session);

    if (errs == 0) {
        fprintf(stdout, " No Errors\n");
    } else {
        fprintf(stderr, "%d Errors\n", errs);
    }

    return MTestReturnValue(errs);
}
