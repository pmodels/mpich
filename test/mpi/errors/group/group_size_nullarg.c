#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    MPI_Group basegroup;
    MPI_Comm comm;
    int rank, size;
    int worldrank;
    int errs = 0, errclass, mpi_errno;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldrank);
    comm = MPI_COMM_WORLD;
    MPI_Comm_group(comm, &basegroup);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* Checking group_size for NULL variable */
    mpi_errno = MPI_Group_size(basegroup, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MPI_Group_free(&basegroup);
    MTest_Finalize(errs);
    return 0;
}
