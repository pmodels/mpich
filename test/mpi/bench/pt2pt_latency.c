#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

/* This test measures a single one-way send/recv latency */

#define MAX_MSG_SIZE 5000000
#define MIN_TIME 1000   /* minimum wall clock for each measurement (us) */
#define NUM_REPEAT 10   /* number of measurements for each message size */
#define WARMUP_THRESH 10        /* the bigger threashold the less likely we are still in warm-up */

/* minimum iteration to get sufficient wall time accuracy */
#define MIN_ITER(latency) ((int) (MIN_TIME / 2 / (latency)))

#define TAG 0

static MPI_Comm comm;
static int comm_rank, comm_size;
static void *buf;

static int test_latency(MPI_Comm comm, int src, int dst, int size);
static double latency_send(MPI_Comm comm, int dst, int size, int iter);
static void latency_recv(MPI_Comm comm, int src, int size);
static int latency_send_warmup(MPI_Comm comm, int dst, int size);

int main(int argc, char **argv)
{
    MPI_Init(NULL, NULL);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &comm_rank);
    MPI_Comm_size(comm, &comm_size);
    buf = malloc(MAX_MSG_SIZE);

    if (comm_rank == 0) {
        printf("# %10s %10s %6s\n", "size", "latency", "err");
    }
    test_latency(MPI_COMM_WORLD, 0, 1, 0);
    for (int size = 1; size < MAX_MSG_SIZE; size *= 2) {
        test_latency(MPI_COMM_WORLD, 0, 1, size);
    }

    MPI_Finalize();
    return 0;
}

/* latency test for a message size */
static int test_latency(MPI_Comm comm, int src, int dst, int size)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    if (rank == src) {
        int iter = latency_send_warmup(comm, dst, size);

        /* do NUM_REPEAT measurements to obtain statistics */
        double sum1 = 0;
        double sum2 = 0;
        for (int i = 0; i < NUM_REPEAT; i++) {
            double t_latency = latency_send(comm, dst, size, iter);
            sum1 += t_latency;
            sum2 += t_latency * t_latency;
        }
        sum1 /= NUM_REPEAT;
        sum2 /= NUM_REPEAT;
        printf("  %10d %10.3f %6.3f\n", size, sum1, sqrt(sum2 - sum1 * sum1));

        /* tell receiver that we are done */
        iter = 0;
        MPI_Send(&iter, 1, MPI_INT, dst, TAG, comm);
    } else if (rank == dst) {
        latency_recv(comm, src, size);
    }
}

/* run latency test until it is warmed up. Return a suggested number of iteratons */
static int latency_send_warmup(MPI_Comm comm, int dst, int size)
{
    /* A charateristics during warm up is the latency monotonically decrease.
     * Keep trying until latency increase multiple times (to eliminate warmup) */
    int iter = 2;
    double last_latency = 1;
    int num_best = 0;
    while (num_best < 10) {
        double t_latency = latency_send(comm, dst, size, iter);
        /* we need sufficient iter to get meaningful wall time measurement */
        if (iter < MIN_ITER(t_latency)) {
            iter = MIN_ITER(t_latency);
            num_best = 0;
            continue;
        }
        /* check that latencies are no longer monotonically decreasing */
        if (t_latency > last_latency) {
            num_best++;
        }
        last_latency = t_latency;
    }
    return iter;
}

/* sender do iter number of ping-pong tests and return the average latency */
static double latency_send(MPI_Comm comm, int dst, int size, int iter)
{
    /* synchronize with receiver */
    MPI_Send(&iter, 1, MPI_INT, dst, TAG, comm);

    double t_start, t_dur;
    double t_latency;

    t_start = MPI_Wtime();
    for (int i = 0; i < iter; i++) {
        MPI_Send(buf, size, MPI_CHAR, dst, TAG, comm);
        MPI_Recv(buf, size, MPI_CHAR, dst, TAG, comm, MPI_STATUS_IGNORE);
    }
    t_dur = MPI_Wtime() - t_start;
    return t_dur * 1e6 / (2.0 * iter);
}

/* receiver keeps serve ping-pong bounce until told to quit */
static void latency_recv(MPI_Comm comm, int src, int size)
{
    while (1) {
        int iter;

        /* synchronize with receiver */
        MPI_Recv(&iter, 1, MPI_INT, src, TAG, comm, MPI_STATUS_IGNORE);
        if (iter == 0) {
            /* time to quit */
            break;
        }

        for (int i = 0; i < iter; i++) {
            MPI_Recv(buf, size, MPI_CHAR, src, TAG, comm, MPI_STATUS_IGNORE);
            MPI_Send(buf, size, MPI_CHAR, src, TAG, comm);
        }
    }
}
