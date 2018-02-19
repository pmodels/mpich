#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0, mpi_errno, errclass, rank, size;
    MPI_Comm dup_comm;
    MPI_Group group;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);
    MPI_Comm_group(dup_comm, &group);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /*test comm_create_group for NULL variable */
    mpi_errno = MPI_Comm_create_group(dup_comm, group, 10, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&dup_comm);
    MPI_Group_free(&group);
    MTest_Finalize(errs);
    return 0;
}
