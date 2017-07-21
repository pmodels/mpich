#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char* argv[])
{
    MPI_Comm comm, newcomm, scomm, intercomm;
    MPI_Group group;
    int rank, size, color, newrank;
    int errs = 0, errclass, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_group(comm, &group);

    MPI_Comm_create(comm, group, &newcomm);
    color = rank % 2;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &scomm);
    MPI_Intercomm_create(scomm, 0, MPI_COMM_WORLD, 1-color, 52, &intercomm);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(intercomm, &newrank);
    /*test comm_remote_size for NULL variable */
    mpi_errno = MPI_Comm_remote_size(intercomm, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MPI_Comm_free(&newcomm);
    MPI_Comm_free(&scomm);
    MPI_Comm_free(&intercomm);
    MPI_Group_free(&group);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

