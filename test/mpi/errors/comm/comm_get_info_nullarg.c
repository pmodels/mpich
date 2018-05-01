#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0, errclass, mpi_errno;
    int rank, size;
    MPI_Comm dup_comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    /*test comm_get_info for NULL variable */
    mpi_errno = MPI_Comm_get_info(dup_comm, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&dup_comm);
    MTest_Finalize(errs);
    return 0;
}
