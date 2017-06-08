/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2013 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

/* 3D Halo Exchange Benchmark.
 * This test measures 3D Halo Exchange by using MPI Derived Datatype. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include <omp.h>

#define X 0
#define Y 1
#define Z 2
#define L 0
#define R 1
#define SND 0
#define RCV 1
#define DX 2
#define DY 1
#define DZ 0
#define NARGSL 8

/* #define DEBUG */
#ifdef DEBUG
#define NARGSU NARGSL + 1
#else
#define NARGSU NARGSL
#endif


int rank, size;
int debug = 0;
double *data = NULL;            /* Data buffer */
double *dst[3][2], *src[3][2];  /* Destination ptr */
/* Receive in main buffer; send from temporary (Y needs (un)packing anyway) */
int neighs[3][2];               /* [X, Y, Z][L,R] */
int delems[3];
int ranks[3];
MPI_Datatype ddt[3];
MPI_Datatype ddt_col;
MPI_Comm comm;
int dims, reps;
int value;
int sizeofdouble = sizeof(double);
#ifdef PROF_INTRA
int *locations = NULL;
#endif

enum type {
    pack, unpack
};
enum method {
    plain = 1, cart_create, cart_create_reorder, custom, plainswapped
};

static inline void allocate(double **buf, size_t delems)
{
    *buf = (double *) malloc(delems * sizeof(double));
    if (*buf == NULL) {
        fprintf(stderr, "Error allocating data\n");
        abort();
    }
}

static inline void fillddt(MPI_Datatype * ddt, int *delems, size_t dims)
{
    if (dims == 3) {
        MPI_Type_vector(delems[DY] - 2, 1, delems[DX], MPI_DOUBLE, &ddt_col);
        MPI_Type_commit(&ddt_col);

        MPI_Type_hvector(delems[DZ] - 2, 1, delems[DX] * delems[DY] * sizeofdouble, ddt_col,
                         &ddt[X]);
        MPI_Type_commit(&ddt[X]);

        MPI_Type_vector(delems[DZ] - 2, delems[DX] - 2, delems[DX] * delems[DY],
                        MPI_DOUBLE, &ddt[Y]);
        MPI_Type_commit(&ddt[Y]);
        MPI_Type_vector(delems[DY] - 2, delems[DX] - 2, delems[DX], MPI_DOUBLE, &ddt[Z]);
        MPI_Type_commit(&ddt[Z]);
    }
    else {
        MPI_Type_vector(delems[DY] - 2, 1, delems[DX], MPI_DOUBLE, &ddt[X]);
        MPI_Type_commit(&ddt[X]);
        MPI_Type_contiguous(delems[DX] - 2, MPI_DOUBLE, &ddt[Y]);
        MPI_Type_commit(&ddt[Y]);
    }
}

#ifdef PROF_INTRA
static void fillRankLocation(int *rank_locations)
{
    MPI_Comm shm_comm = MPI_COMM_NULL;
    int node_id = -1, shm_rank = 0;
    int i;

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shm_comm);
    MPI_Comm_rank(shm_comm, &shm_rank);

    /* Use the first local process's world rank as node id */
    if (shm_rank == 0)
        node_id = rank;
    MPI_Bcast(&node_id, 1, MPI_INT, 0, shm_comm);

    /* Exchange node id in world */
    rank_locations[rank] = node_id;
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, rank_locations, 1, MPI_INT, MPI_COMM_WORLD);

    /* Simply set remote rank to 1 and local rank to 0. */
    for (i = 0; i < size; i++) {
        if (rank_locations[i] != node_id) {
            rank_locations[i] = 1;
        }
        else {
            rank_locations[i] = 0;
        }
    }

    if(rank == 0){
        printf("locations:");
        for (i = 0; i < size; i++) {
            printf(" %d ", rank_locations[i]);
        }
        printf("\n");
        fflush(stdout);
    }
    MPI_Comm_free(&shm_comm);

}
#endif

static inline void _plain(int (*neighs)[2], int *ranks, int rank)
{
    int plane, d2, coords[3];

    plane = ranks[X] * ranks[Y];
    coords[Z] = rank / plane;
    d2 = rank % plane;
    coords[Y] = d2 / ranks[X];
    coords[X] = d2 % ranks[X];

    neighs[X][L] = coords[X] > 0 ? rank - 1 : rank + ranks[X] - 1;      /* W */
    neighs[X][R] = coords[X] < ranks[X] - 1 ? rank + 1 : rank - ranks[X] + 1;   /* E */
    neighs[Y][L] = coords[Y] > 0 ? rank - ranks[X] : rank + ranks[X] * (ranks[Y] - 1);  /* N */
    neighs[Y][R] = coords[Y] < ranks[Y] - 1 ? rank + ranks[X] : rank - ranks[X] * (ranks[Y] - 1);       /* S */
    neighs[Z][L] = coords[Z] > 0 ? rank - plane : rank + plane * (ranks[Z] - 1);
    neighs[Z][R] = coords[Z] < ranks[Z] - 1 ? rank + plane : rank - plane * (ranks[Z] - 1);
}

static void fillNeighs(int (*neighs)[2], int *ranks, enum method m, MPI_Comm * comm, int *populate)
{
    switch (m) {
    case plain:
        _plain(neighs, ranks, rank);
        *comm = MPI_COMM_WORLD;
        *populate = rank;
        break;

    default:
        abort();
        break;
    }
}

static void populate(double *data, size_t amount, int value)
{
    size_t i;
    for (i = 0; i < amount; i++)
        data[i] = value;
}

#ifdef DEBUG
static void display(double *data, int *delems)
{
    size_t k, j, i;
    for (k = 0; k < delems[DZ]; k++) {
        for (j = 0; j < delems[DY]; j++) {
            for (i = 0; i < delems[DX]; i++) {
                fprintf(stderr, "%.2f ", *data);
                data++;
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}
#endif

static int check2d(double *data, int *delems, int (*neighs)[2], double value)
{
    double *ptr, *ptr2;
    int err = 0;

    /* Edges */
    ptr = data;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 0] Rank %d data mismatch in (0,0,0) %.2f != %.2f\n",
                rank, *ptr, value);
        err++;
    }
    ptr = ptr + delems[DX] - 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 1] Rank %d data mismatch in (%d,0,0) %.2f != %.2f\n",
                rank, delems[DX] - 1, *ptr, value);
        err++;
    }
    ptr = ptr + delems[DX] * (delems[DY] - 1);
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 2] Rank %d data mismatch in (%d,%d,0) %.2f != %.2f\n",
                rank, delems[DX] - 1, delems[DY] - 1, *ptr, value);
        err++;
    }
    ptr = ptr - delems[DX] + 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 3] Rank %d data mismatch in (0,%d,0) %.2f != %.2f\n",
                rank, delems[DY] - 1, *ptr, value);
        err++;
    }

    /* X lines */
    ptr = data + 1;
    ptr2 = data + delems[DX] * (delems[DY] - 1) + 1;
    int i;
    for (i = 1; i < delems[DX] - 1; i++) {
        if (*ptr != (double) neighs[Y][L]) {
            fprintf(stderr,
                    "[External X lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, 0, 0, *ptr, (double) neighs[Y][L]);
            err++;
        }
        if (*ptr2 != (double) neighs[Y][R]) {
            fprintf(stderr,
                    "[External X lines 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, delems[DY] - 1, 0, *ptr2, (double) neighs[Y][R]);
            err++;
        }
        ptr++;
        ptr2++;
    }

    /* Y lines */
    ptr = data + delems[DX];
    ptr2 = data + delems[DX] * 2 - 1;
    int j;
    for (j = 1; j < delems[DY] - 1; j++) {
        if (*ptr != (double) neighs[X][L]) {
            fprintf(stderr,
                    "[External Y lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, 0, j, 0, *ptr, (double) neighs[X][L]);
            err++;
        }

        if (*ptr2 != (double) neighs[X][R]) {
            fprintf(stderr,
                    "[External Y lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, delems[DX] - 1, j, 0, *ptr2, (double) neighs[X][R]);
            err++;
        }

        ptr += delems[DX];
        ptr2 += delems[DX];
    }
    if (err > 0) {
#ifdef DEBUG
        if (rank == debug && delems[DX] < 8)
            display(data, delems);
#endif
    }
    return err;
}

static int check3d(double *data, int *delems, int (*neighs)[2], double value)
{
    double *ptr, *ptr2, *ptr3, *ptr4;
    int i, j, k, err = 0;

    /* Front edges */
    ptr = data;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, 0, 0, 0, *ptr, value);
        err++;
    }

    ptr = ptr + delems[DX] - 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, delems[DX] - 1, 0, 0, *ptr, value);
        err++;
    }

    ptr = ptr + delems[DX] * (delems[DY] - 1);
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 2] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, delems[DX] - 1, delems[DY] - 1, 0, *ptr, value);
        err++;
    }

    ptr = ptr - delems[DX] + 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Front edges 3] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, 0, delems[DY] - 1, 0, *ptr, value);
        err++;
    }

    /* Back edges */
    ptr = data + delems[DX] * delems[DY] * (delems[DZ] - 1);
    if (*ptr != value) {
        fprintf(stderr,
                "[Back edges 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, 0, 0, delems[DZ] - 1, *ptr, value);
        err++;
    }

    ptr = ptr + delems[DX] - 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Back edges 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, delems[DX] - 1, 0, delems[DZ] - 1, *ptr, value);
        err++;
    }

    ptr = ptr + delems[DX] * (delems[DY] - 1);
    if (*ptr != value) {
        fprintf(stderr,
                "[Back edges 2] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, delems[DX] - 1, delems[DY] - 1, delems[DZ] - 1, *ptr, value);
        err++;
    }

    ptr = ptr - delems[DX] + 1;
    if (*ptr != value) {
        fprintf(stderr,
                "[Back edges 3] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                rank, 0, delems[DY] - 1, delems[DZ] - 1, *ptr, value);
        err++;
    }

    /* External X lines */
    ptr = data + 1;
    ptr2 = data + delems[DX] * (delems[DY] - 1) + 1;
    ptr3 = data + delems[DX] * delems[DY] * (delems[DZ] - 1) + 1;
    ptr4 = data + delems[DX] * delems[DY] * (delems[DZ] - 1)
        + delems[DX] * (delems[DY] - 1) + 1;
    for (i = 1; i < delems[DX] - 1; i++) {
        if (*ptr != value) {
            fprintf(stderr,
                    "[External X lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, 0, 0, *ptr, value);
            err++;
        }

        if (*ptr2 != value) {
            fprintf(stderr,
                    "[External X lines 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, delems[DY] - 1, 0, *ptr2, value);
            err++;
        }

        if (*ptr3 != value) {
            fprintf(stderr,
                    "[External X lines 2] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, 0, delems[DZ] - 1, *ptr3, value);
            err++;
        }

        if (*ptr4 != value) {
            fprintf(stderr,
                    "[External X lines 3] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, i, delems[DY] - 1, delems[DZ] - 1, *ptr4, value);
            err++;
        }
        ptr++;
        ptr2++;
        ptr3++;
        ptr4++;
    }

    /* External Y lines */
    ptr = data + delems[DX];
    ptr2 = data + delems[DX] * 2 - 1;
    ptr3 = data + delems[DX] * delems[DY] * (delems[DZ] - 1) + delems[DX];
    ptr4 = data + delems[DX] * delems[DY] * (delems[DZ] - 1) + delems[DX] * 2 - 1;
    for (j = 1; j < delems[DY] - 1; j++) {
        if (*ptr != value) {
            fprintf(stderr,
                    "[External Y lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, 0, j, 0, *ptr, value);
            err++;
        }

        if (*ptr2 != value) {
            fprintf(stderr,
                    "[External Y lines 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, delems[DX] - 1, j, 0, *ptr2, value);
            err++;
        }
        if (*ptr3 != value) {
            fprintf(stderr,
                    "[External Y lines 2] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, 0, j, delems[DZ] - 1, *ptr3, value);
            err++;
        }
        if (*ptr4 != value) {
            fprintf(stderr,
                    "[External Y lines 3] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, delems[DX] - 1, j, delems[DZ] - 1, *ptr4, value);
            err++;
        }
        ptr += delems[DX];
        ptr2 += delems[DX];
        ptr3 += delems[DX];
        ptr4 += delems[DX];
    }

    /* External Z lines */
    ptr = data + delems[DX] * delems[DY];
    ptr2 = data + delems[DX] * delems[DY] + delems[DX] - 1;
    ptr3 = data + delems[DX] * delems[DY] + delems[DX] * (delems[DY] - 1);
    ptr4 = data + delems[DX] * delems[DY] + delems[DX] * (delems[DY] - 1)
        + delems[DX] - 1;
    for (k = 1; k < delems[DZ] - 1; k++) {
        if (*ptr != value) {
            fprintf(stderr,
                    "[External Z lines 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, 0, 0, k, *ptr, value);
            err++;
        }
        if (*ptr2 != value) {
            fprintf(stderr,
                    "[External Z lines 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, delems[DX] - 1, 0, k, *ptr2, value);
            err++;
        }
        if (*ptr3 != value) {
            fprintf(stderr,
                    "[External Z lines 2] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, 0, delems[DY] - 1, k, *ptr3, value);
            err++;
        }
        if (*ptr4 != value) {
            fprintf(stderr,
                    "[External Z lines 3] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                    rank, delems[DX] - 1, delems[DY] - 1, k, *ptr4, value);
            err++;
        }
        ptr += delems[DX] * delems[DY];
        ptr2 += delems[DX] * delems[DY];
        ptr3 += delems[DX] * delems[DY];
        ptr4 += delems[DX] * delems[DY];
    }

    /* External XY planes */
    ptr = data + delems[DX];
    ptr2 = data + delems[DX] * delems[DY] * (delems[DZ] - 1) + delems[DX];
    for (j = 1; j < delems[DY] - 1; j++) {
        ptr++;
        ptr2++;
        for (i = 1; i < delems[DX] - 1; i++) {
            if (*ptr != (double) neighs[Z][L]) {
                fprintf(stderr,
                        "[External XY planes 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, i, j, 0, *ptr, (double) neighs[Z][L]);
                err++;
            }
            if (*ptr2 != (double) neighs[Z][R]) {
                fprintf(stderr,
                        "[External XY planes 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, i, j, delems[DZ] - 1, *ptr2, (double) neighs[Z][R]);
                err++;
            }
            ptr++;
            ptr2++;
        }
        ptr++;
        ptr2++;
    }

    /* External XZ planes */
    ptr = data + delems[DX] * delems[DY];
    ptr2 = data + delems[DX] * delems[DY] + delems[DX] * (delems[DY] - 1);
    for (k = 1; k < delems[DZ] - 1; k++) {
        ptr++;
        ptr2++;
        for (i = 1; i < delems[DX] - 1; i++) {
            if (*ptr != (double) neighs[Y][L]) {
                fprintf(stderr,
                        "[External XZ planes 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, i, 0, k, *ptr, (double) neighs[Y][L]);
                err++;
            }
            if (*ptr2 != (double) neighs[Y][R]) {
                fprintf(stderr,
                        "[External XZ planes 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, i, delems[DY] - 1, k, *ptr2, (double) neighs[Y][R]);
                err++;
            }
            ptr++;
            ptr2++;
        }
        ptr += delems[DX] * (delems[DY] - 1) + 1;
        ptr2 += delems[DX] * (delems[DY] - 1) + 1;
    }

    /* External YZ planes */
    ptr = data + delems[DX] * delems[DY];
    ptr2 = data + delems[DX] * delems[DY] + delems[DX] - 1;
    for (k = 1; k < delems[DZ] - 1; k++) {
        ptr += delems[DX];
        ptr2 += delems[DX];
        for (j = 1; j < delems[DY] - 1; j++) {
            if (*ptr != (double) neighs[X][L]) {
                fprintf(stderr,
                        "[External YZ planes 0] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, 0, j, k, *ptr, (double) neighs[X][L]);
                err++;
            }
            if (*ptr2 != (double) neighs[X][R]) {
                fprintf(stderr,
                        "[External YZ planes 1] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                        rank, delems[DX] - 1, j, k, *ptr2, (double) neighs[X][R]);
                err++;
            }
            ptr += delems[DX];
            ptr2 += delems[DX];
        }
        ptr += delems[DX];
        ptr2 += delems[DX];
    }

    /* Inner elements */
    ptr = data + delems[DX] * delems[DY];
    for (k = 1; k < delems[DZ] - 1; k++) {
        ptr += delems[DX];
        for (j = 1; j < delems[DY] - 1; j++) {
            ptr++;
            for (i = 1; i < delems[DX] - 1; i++) {
                if (*ptr != value) {
                    {
                        fprintf(stderr,
                                "[Inner elements] Rank %d data mismatch in (%d,%d,%d) %.2f != %.2f\n",
                                rank, i, j, k, *ptr, value);
                        err++;
                    }
                }
                ptr++;
            }
            ptr++;
        }
        ptr += delems[DX];
    }

    if (err > 0) {
#ifdef DEBUG
        if (rank == debug && delems[DX] < 8)
            display(data, delems);
#endif
    }
    return err;
}

static void finish(const char *msg)
{
    if (rank == 0)
        fprintf(stderr, "%s\n", msg);
    MPI_Finalize();
    exit(1);
}

#define RUN_ITER                        \
    for (i = 0; i < reps; i++) {        \
        reqn = 0;                       \
        for (j = 0; j < dims; j++)      \
            for (k = 0; k < 2; k++)     \
                MPI_Irecv(dst[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[reqn++]);   \
        for (j = 0; j < dims; j++)      \
            for (k = 0; k < 2; k++)     \
                MPI_Isend(src[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[reqn++]);   \
        MPI_Waitall(dims * 4, request, statuses);       \
    }

#define RUN_ITER_PROF_DIM               \
        for (i = 0; i < reps; i++) {    \
            for (k = 0; k < 2; k++) {   \
                MPI_Irecv(dst[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[k*2]);      \
                MPI_Isend(src[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[k*2+1]);    \
            }                           \
            MPI_Waitall(2, request, statuses);  \
        }

#define RUN_ITER_PROF_INTRA                 \
        for (i = 0; i < reps; i++) {        \
            reqn = 0;                       \
            for (j = 0; j < dims; j++) {    \
                for (k = 0; k < 2; k++) {   \
                    int loc = locations[neighs[j][k]];                                          \
                    if (loc == 1) { intra_nplane[1]++; continue; }                              \
                    MPI_Irecv(dst[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[reqn++]);   \
                    intra_nplane[0]++;                                                          \
                }   \
            }       \
            for (j = 0; j < dims; j++) {                \
                for (k = 0; k < 2; k++) {               \
                    int loc = locations[neighs[j][k]];  \
                    if (loc == 1) { continue; }         \
                    MPI_Isend(src[j][k], 1, ddt[j], neighs[j][k], 0, comm, &request[reqn++]);   \
                }   \
            }       \
            MPI_Waitall(reqn, request, statuses);       \
        }

static int run_iter(void)
{
    MPI_Request request[12];    /* S/R X Y */
    MPI_Status statuses[12];
    double t0, t1, t2, agv_t;
    int i, j, k;
    agv_t = 0.0;
    int reqn = 0;
    int err = 0;
#ifdef PROF_DIM_TIME
    double dim_ts[3] = { 0, 0, 0 }, dim_ts_avg[3] = {
    0, 0, 0};
    double dim_t0;
#endif
    MPI_Barrier(comm);
#ifdef PROF_INTRA
    int intra_nplane[2] = {0}, intra_nplane_avg[2] = {0};
#endif

    t0 = MPI_Wtime();
#ifdef PROF_DIM_TIME
    for (j = 0; j < dims; j++) {
        dim_t0 = MPI_Wtime();
        RUN_ITER_PROF_DIM;
        dim_ts[j] = MPI_Wtime() - dim_t0;
    }
#elif defined(PROF_INTRA)
    for (j = 0; j < dims; j++) {
        RUN_ITER_PROF_INTRA;
    }
#else
    RUN_ITER;
#endif
    t1 = MPI_Wtime() - t0;
    MPI_Reduce(&t1, &t2, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    agv_t = t2 / size;

#ifdef PROF_DIM_TIME
    MPI_Reduce(dim_ts, dim_ts_avg, 3, MPI_DOUBLE, MPI_SUM, 0, comm);
#elif defined(PROF_INTRA)
    MPI_Reduce(intra_nplane, intra_nplane_avg, 2, MPI_INT, MPI_SUM, 0, comm);
#endif

#ifndef PROF_INTRA /*skip check*/
    if (dims == 2)
        err = check2d(data, delems, neighs, (double) value);
    else
        err = check3d(data, delems, neighs, (double) value);

    if (err) {
        finish("check error");
    }
#endif

#if 0
#ifdef PROF_INTRA /* always x-y planes [2][0,1] are remote, if set rz small.*/
    for (i = 0; i< size; i++) {
        if(rank == i) {
            fprintf(stdout, "[%d] remote plans:", rank);
            for (j = 0; j < dims; j++) {
                for (k = 0; k < 2; k++) {
                    int loc = locations[neighs[j][k]];
                    if(loc == 1) {
                        fprintf(stdout, "[%d,%d] ", j, k);
                    }
                }
            }
            fprintf(stdout, "\n");
            fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
#endif
#endif

    if (rank == 0) {
        agv_t /= reps;
#ifdef PROF_DIM_TIME
        for (j = 0; j < dims; j++) {
            dim_ts_avg[j] = (dim_ts_avg[j] / size * 1000 * 1000 / reps);
        }
#elif defined(PROF_INTRA)
        for (j = 0; j < 2; j++)
            intra_nplane_avg[j] = (intra_nplane_avg[j] / size / reps);
#endif

        fprintf(stdout, "%d, %d; %d, %d, %d; %d, %d, %d; %.4lf"
#ifdef PROF_DIM_TIME
                " yz %.4lf, xz %.4lf, xy %.4lf"
#elif defined(PROF_INTRA)
                " intra_ex %d, inter_ex(skipped) %d"
#endif
                "\n", size,
                reps, delems[DX], delems[DY], delems[DZ], ranks[X],
                ranks[Y], ranks[Z], agv_t * 1000 * 1000
#ifdef PROF_DIM_TIME
                , dim_ts_avg[0], dim_ts_avg[1], dim_ts_avg[2]
#elif defined(PROF_INTRA)
                , intra_nplane_avg[0], intra_nplane_avg[1]
#endif
);
    }

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    char *endptr;
    enum method m = plain;
    MPI_Info info = MPI_INFO_NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#ifdef BATCH
    reps = 100;

    ranks[X] = RANK_X;
    ranks[Y] = RANK_Y;
    ranks[Z] = RANK_Z;
    if (size != ranks[X] * ranks[Y] * ranks[Z])
        finish("Rank dimensions don't match COMM_WORLD size.");
    delems[DX] = DELEM_X;
    delems[DY] = DELEM_Y;
    delems[DZ] = DELEM_Z;

    dims = ranks[Z] > 1 ? 3 : 2;
#else
    if (argc < NARGSL || argc > NARGSU) {
        finish
            ("Usage: <number of repetitions> <X ranks> <Y ranks> <Z ranks> <X doubles> <Y doubles> <Z doubles> [verbose rank]");
    }

    reps = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0')
        finish("Error parsing number of repetitions argument.");
    ranks[X] = strtol(argv[2], &endptr, 10);    /* X ranks */
    if (*endptr != '\0')
        finish("Error parsing X dimension elements argument.");
    ranks[Y] = strtol(argv[3], &endptr, 10);    /* Y ranks */
    if (*endptr != '\0')
        finish("Error parsing Y dimension elements argument.");
    ranks[Z] = strtol(argv[4], &endptr, 10);    /* Z ranks */
    if (size != ranks[X] * ranks[Y] * ranks[Z])
        finish("Rank dimensions don't match COMM_WORLD size.");
    if (*endptr != '\0')
        finish("Error parsing Z dimension elements argument.");
    delems[DX] = strtol(argv[5], &endptr, 10);  /* X dimension elements */
    if (*endptr != '\0')
        finish("Error parsing X dimension elements argument.");
    delems[DY] = strtol(argv[6], &endptr, 10);  /* Y dimension elements */
    if (*endptr != '\0')
        finish("Error parsing Y dimension elements argument.");
    delems[DZ] = strtol(argv[7], &endptr, 10);  /* Z dimension elements */
    if (*endptr != '\0')
        finish("Error parsing Z dimension elements argument.");
    if (*endptr != '\0' || m < plain || m > plainswapped)
        finish("Error parsing method argument.");

    dims = ranks[Z] > 1 ? 3 : 2;
#endif

#ifdef DEBUG
    int debug = 0;
    if (argc == NARGSU)
        debug = strtol(argv[argc - 1], &endptr, 10);
#endif

    MPI_Info_create(&info);

#ifdef SET_SYMM_INFO
    /* All communication use the same datatype on sender and receiver. */
    MPI_Info_set(info, (char *) "symm_datatype", (char *) "true");
    MPI_Comm_set_info(MPI_COMM_WORLD, info);
#endif

    allocate(&data, (size_t) delems[DX] * delems[DY] * delems[DZ]);
    fillddt(ddt, delems, dims);

#ifdef PROF_INTRA
    locations = malloc(size * sizeof(int));
    fillRankLocation(locations);
#endif

    size_t plane = dims == 3 ? delems[DX] * delems[DY] : 0;
    dst[X][L] = data + delems[DX] + plane;
    dst[X][R] = data + delems[DX] + plane + delems[DX] - 1;
    dst[Y][L] = data + 1 + plane;
    dst[Y][R] = data + 1 + plane + delems[DX] * (delems[DY] - 1);
    dst[Z][L] = data + delems[DX] + 1;
    dst[Z][R] = data + delems[DX] + 1 + plane * (delems[DZ] - 1);

    src[X][L] = dst[X][L] + 1;
    src[X][R] = dst[X][R] - 1;
    src[Y][L] = dst[Y][L] + delems[DX];
    src[Y][R] = dst[Y][R] - delems[DX];
    src[Z][L] = dst[Z][L] + plane;
    src[Z][R] = dst[Z][R] - plane;

    fillNeighs(neighs, ranks, m, &comm, &value);
    populate(data, delems[DX] * delems[DY] * delems[DZ], value);

    run_iter();

    for (i = 0; i < dims; i++)
        MPI_Type_free(&ddt[i]);
    if (dims == 3)
        MPI_Type_free(&ddt_col);

    MPI_Info_free(&info);
    MPI_Finalize();

#ifdef DEBUG
    if (rank == debug && delems[DX] < 8)
        display(data, delems);
#endif

    if (data)
        free(data);

#ifdef PROF_INTRA
    if (locations)
        free(locations);
#endif
    return 0;
}
