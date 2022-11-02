/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

int main(void)
{
    int mpi_errno = MPI_SUCCESS;
    int errs = 0;

    MTest_Init(NULL, NULL);

    /* Current MPICH may not support thread multiple, especially with pervci */
    int thread_level;
    MPI_Query_thread(&thread_level);
    if (thread_level == MPI_THREAD_MULTIPLE) {
        goto fn_exit;
    }

    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
    /* The implementation may raise an error on the default handler
     * since it will happen during MPI_Comm_free. The default,
     * as of MPI-4.0, is the error handler for MPI_COMM_SELF.
     * Prior to MPI-4.0, it was MPI_COMM_WORLD. */
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    int rank;
    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
        /* if an implementation does not use eager
         * protocol, this send may hang */
        MPI_Send(NULL, 0, MPI_INT, 1, 0, comm);
    }

    MPI_Barrier(comm);

    mpi_errno = MPI_Comm_free(&comm);
    /* unmatched message should be detected at rank 1 */
    if (rank == 1 && mpi_errno == MPI_SUCCESS) {
        errs++;
    }

  fn_exit:
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
