#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

/* This test measures point-to-point isend/irecv bandwidth */

#define MAX_MSG_SIZE 5000000
#define MIN_TIME 1e-3   /* minimum wall clock for each measurement (in sec) */
#define NUM_REPEAT 10   /* number of measurements for each message size */
#define WARMUP_THRESH 10        /* the bigger threashold the less likely we are still in warm-up */
#define MAX_BATCH_SIZE 1024

/* minimum iteration to get sufficient wall time accuracy */
#define MIN_ITER(latency) ((int) (MIN_TIME / 2 / (latency)))

#define TAG 0
#define SYNC_TAG 100

static MPI_Comm comm;
static int comm_rank, comm_size;
static void *buf;

static int test_bw(MPI_Comm comm, int src, int dst, int size);
static double bw_send(MPI_Comm comm, int dst, int size, int batch_size, int num_batches);
static void bw_recv(MPI_Comm comm, int src, int size);
static void bw_send_warmup(MPI_Comm comm, int dst, int size,
                           int *batch_size_out, int *num_batches_out);

int main(int argc, char **argv)
{
    MPI_Init(NULL, NULL);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &comm_rank);
    MPI_Comm_size(comm, &comm_size);
    buf = malloc(MAX_MSG_SIZE);

    if (comm_rank == 0) {
        printf("# %10s %10s %11s %8s\n", "size", "bandwidth", "avg.latency", "err");
    }
    for (int size = 1; size < MAX_MSG_SIZE; size *= 2) {
        test_bw(MPI_COMM_WORLD, 0, 1, size);
    }

    MPI_Finalize();
    return 0;
}

/* bandwidth test for a message size */
static int test_bw(MPI_Comm comm, int src, int dst, int size)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    if (rank == src) {
        int batch_size, num_batches;
        bw_send_warmup(comm, dst, size, &batch_size, &num_batches);

        /* do NUM_REPEAT measurements to obtain statistics */
        double sum_bw = 0;
        double sum1 = 0;
        double sum2 = 0;
        for (int i = 0; i < NUM_REPEAT; i++) {
            double t_dur = bw_send(comm, dst, size, batch_size, num_batches);
            double t_bw = size / 1e6 * num_batches * batch_size / t_dur;
            double t_latency = t_dur * 1e6 / (batch_size * num_batches);

            sum_bw += t_bw;
            sum1 += t_latency;
            sum2 += t_latency * t_latency;
        }
        sum_bw /= NUM_REPEAT;
        sum1 /= NUM_REPEAT;
        sum2 /= NUM_REPEAT;
        printf("  %10d %10.3f %11.3f %8.3f\n", size, sum_bw, sum1, sqrt(sum2 - sum1 * sum1));

        /* tell receiver that we are done */
        int params[2] = { 0, 0 };
        MPI_Send(params, 2, MPI_INT, dst, SYNC_TAG, comm);
    } else if (rank == dst) {
        bw_recv(comm, src, size);
    }
}

/* run bandwidth test until it is warmed up. Return a suggested batch_size and num_batches */
static void bw_send_warmup(MPI_Comm comm, int dst, int size,
                           int *batch_size_out, int *num_batches_out)
{
    /* A charateristics during warm up is the bandwidth monotonically increase.
     * Keep trying until bandwidth increase multiple times (to eliminate warmup) */
    int batch_size = 128;
    int num_batches = 1;
    double last_dur = 1;
    int num_best = 0;
    while (num_best < 10) {
        double t_dur = bw_send(comm, dst, size, batch_size, num_batches);
        /* we need sufficient num_batches to get meaningful wall time measurement */
#define MIN_BATACHES(dur) ((int) (num_batches * (MIN_TIME / t_dur)) + 1)
        if (num_batches < MIN_BATACHES(dur)) {
            num_batches = MIN_BATACHES(dur);
            num_best = 0;
            continue;
        }
        /* check that latencies are no longer monotonically decreasing */
        if (t_dur > last_dur) {
            num_best++;
        }
        last_dur = t_dur;
    }
    /* TODO: optimize batch_size */
    *batch_size_out = batch_size;
    *num_batches_out = num_batches;
}

/* sender do iter number of ping-pong tests and return the average bandwidth */
static double bw_send(MPI_Comm comm, int dst, int size, int batch_size, int num_batches)
{
    /* synchronize with receiver */
    int params[2];
    params[0] = batch_size;
    params[1] = num_batches;
    MPI_Send(params, 2, MPI_INT, dst, SYNC_TAG, comm);

    double t_start, t_dur;
    double t_bw;

    t_start = MPI_Wtime();
    MPI_Request reqs[MAX_BATCH_SIZE];
    for (int i = 0; i < num_batches; i++) {
        for (int j = 0; j < batch_size; j++) {
            MPI_Isend(buf, size, MPI_CHAR, dst, TAG, comm, &reqs[j]);
        }
        MPI_Waitall(batch_size, reqs, MPI_STATUSES_IGNORE);
    }
    MPI_Recv(NULL, 0, MPI_DATATYPE_NULL, dst, TAG + 1, comm, MPI_STATUS_IGNORE);
    t_dur = MPI_Wtime() - t_start;

    return t_dur;
}

/* receiver keeps serve ping-pong bounce until told to quit */
static void bw_recv(MPI_Comm comm, int src, int size)
{
    while (1) {
        int batch_size, num_batches;

        /* synchronize with receiver */
        int params[2];
        MPI_Recv(params, 2, MPI_INT, src, SYNC_TAG, comm, MPI_STATUS_IGNORE);
        batch_size = params[0];
        num_batches = params[1];

        if (batch_size == 0) {
            /* time to quit */
            break;
        }

        MPI_Request reqs[MAX_BATCH_SIZE];
        for (int i = 0; i < num_batches; i++) {
            for (int j = 0; j < batch_size; j++) {
                MPI_Irecv(buf, size, MPI_CHAR, src, TAG, comm, &reqs[j]);
            }
            MPI_Waitall(batch_size, reqs, MPI_STATUSES_IGNORE);
        }
        MPI_Send(NULL, 0, MPI_DATATYPE_NULL, src, TAG + 1, comm);
    }
}
