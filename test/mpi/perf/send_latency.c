#include <stdio.h>
#include <mpi.h>
#include <time.h>

/* This test checks the time it takes for a single send message to be
 * received. Common user probably won't expect a single send will take
 * forever unless the sender do another MPI call to poke the progress.
 *
 * OFI is having trouble with this test.
 */

int main(void)
{
    int rank, size;
    int comm = MPI_COMM_WORLD;
    int tag = 1;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    double T;
    if (rank == 0) {
        T = MPI_Wtime();
        MPI_Send(&T, 1, MPI_DOUBLE, 1, tag, comm);
        T = MPI_Wtime();
        MPI_Send(&T, 1, MPI_DOUBLE, 1, tag + 1, comm);
        /* simulate some computation with sleep */
        struct timespec duration = {2, 0};
        nanosleep(&duration, NULL);
    } else if (rank == 1) {
        int num_errs = 0;
        double latency;

        MPI_Recv(&T, 1, MPI_DOUBLE, 0, tag, comm, MPI_STATUS_IGNORE);
        latency = MPI_Wtime() - T;
        if (latency > 1) {
            num_errs++;
            printf("Recved message 1 in %f seconds\n", latency);
        }
        MPI_Recv(&T, 1, MPI_DOUBLE, 0, tag + 1, comm, MPI_STATUS_IGNORE);
        latency = MPI_Wtime() - T;
        if (latency > 1) {
            num_errs++;
            printf("Recved message 2 in %f seconds\n", latency);
        }

        if (num_errs == 0) {
            printf("No Errors\n");
        }
    }

    MPI_Finalize();
    return 0;
}
