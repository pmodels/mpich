#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    MPI_Group basegroup;
    MPI_Group g1;
    MPI_Comm comm, newcomm;
    int errs = 0, mpi_errno, errclass, rank, size;
    int range[1][3];
    int worldrank;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldrank);
    comm = MPI_COMM_WORLD;
    MPI_Comm_group(comm, &basegroup);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_split(comm, 0, size - rank, &newcomm);
    MPI_Comm_group(newcomm, &g1);

    /* Checking group_range_excl for NULL variable */
    range[0][0] = 1;
    range[0][1] = size - 1;
    range[0][2] = 1;
    mpi_errno = MPI_Group_range_incl(basegroup, 1, range, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MPI_Comm_free(&newcomm);
    MPI_Group_free(&basegroup);
    MPI_Group_free(&g1);
    MTest_Finalize(errs);
    return 0;
}
