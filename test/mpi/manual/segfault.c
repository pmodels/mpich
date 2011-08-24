#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    int *ptr = NULL;
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        (*ptr)++; /* will segfault */
        /* never get below here, just present to prevent dead-code elimination */
        printf("*ptr=%d\n", (*ptr));
    }
    MPI_Finalize();
    return 0;
}

