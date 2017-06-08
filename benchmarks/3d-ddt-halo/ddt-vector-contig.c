/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2017 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

/* Vector to/from contiguous buffer P2P benchmark.
 * 3D Halo vector types are reused. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mpi.h>

#ifdef USE_PAPI
#include "papi.h"

#define PAPI_EVENT_MAX 11
int EventSet = PAPI_NULL;
int event_codes[PAPI_EVENT_MAX] = {
    [0] = PAPI_L1_TCM,
    [1] = PAPI_L2_TCM,
    [2] = PAPI_L3_TCM,
    [4] = PAPI_L1_ICM,
    [5] = PAPI_L1_LDM,
    [6] = PAPI_L1_STM,
    [7] = PAPI_L2_DCA,
    [8] = PAPI_L3_DCA,
    [9] = PAPI_L2_TCA,
    [10] = PAPI_L3_TCA,
};

int event_code = 0;
char event_name[PAPI_MAX_STR_LEN];

long long papi_values[2] = { 0, 0 };
#endif

#define X 0
#define Y 1
#define Z 2
#define L 0
#define R 1
#define DX 2
#define DY 1
#define DZ 0

int rank, size;
double *data = NULL;            /* Data buffer */
double *contig_buf = NULL;
size_t contig_count = 0;
double *dst[3][2], *src[3][2];  /* Destination ptr */
/* Receive in main buffer; send from temporary (Y needs (un)packing anyway) */
int delems[3];
int ranks[3];
MPI_Datatype ddt[3];
MPI_Datatype ddt_col = MPI_DATATYPE_NULL;
int dims = 3, reps = 100, test_dim = X;
int sizeofdouble = sizeof(double);
const char *dim_str[3] = { "X", "Y", "Z" };

#ifdef CONTIG_TO_VEC
const char *test_name = "contig_to_vec";
#else
const char *test_name = "vec_to_contig";
#endif

#ifdef USE_PAPI
int papi_exit()
{
    int retval;

    retval = PAPI_cleanup_eventset(EventSet);
    if (retval != PAPI_OK) {
        PAPI_perror("PAPI_cleanup_eventset");
        return -1;
    }

    retval = PAPI_destroy_eventset(&EventSet);
    if (retval != PAPI_OK) {
        PAPI_perror("PAPI_destroy_eventset");
        return -1;
    }

    PAPI_shutdown();
    return 0;
}


int papi_init()
{
    int retval;

    if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
        PAPI_perror("PAPI_library_init");
        return -1;
    }

    if ((retval = PAPI_create_eventset(&EventSet)) != PAPI_OK) {
        PAPI_perror("PAPI_create_eventset");
        return -1;
    }

    if ((retval = PAPI_add_event(EventSet, event_code)) != PAPI_OK) {
        PAPI_perror("PAPI_add_env_event");
        return -1;
    }

    if ((retval = PAPI_event_code_to_name(event_code, event_name)) != PAPI_OK) {
        PAPI_perror("PAPI_event_code_to_name");
        return -1;
    }

    return 0;
}

#define PAPI_STOP()  do {                                               \
        int retval;                                                     \
        if ((retval = PAPI_stop(EventSet, &papi_values[rank])) != PAPI_OK) {   \
            PAPI_perror("PAPI_stop");                                   \
        }                                                               \
} while (0)

#define PAPI_START()  do {                                               \
        int retval;                                                     \
        if ((retval = PAPI_start(EventSet)) != PAPI_OK) {   \
            PAPI_perror("PAPI_start");                                   \
        }                                                               \
} while (0)
#endif

static inline void allocate(double **buf, size_t delems)
{
    *buf = (double *) malloc(delems * sizeof(double));
    if (*buf == NULL) {
        fprintf(stderr, "Error allocating data\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
}

static inline void prepare_contig(double **buf, int test_dim)
{
    int dim_size = 0;

    MPI_Type_size(ddt[test_dim], &dim_size);

    *buf = (double *) malloc(dim_size);
    if (*buf == NULL) {
        fprintf(stderr, "Error allocating contig buffer\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    contig_count = (size_t) (dim_size / sizeof(double));
}

static inline void fillddt(MPI_Datatype * ddt, int *delems)
{
    MPI_Type_vector(delems[DY] - 2, 1, delems[DX], MPI_DOUBLE, &ddt_col);
    MPI_Type_commit(&ddt_col);

    MPI_Type_hvector(delems[DZ] - 2, 1, delems[DX] * delems[DY] * sizeofdouble, ddt_col, &ddt[X]);
    MPI_Type_commit(&ddt[X]);

    MPI_Type_vector(delems[DZ] - 2, delems[DX] - 2, delems[DX] * delems[DY], MPI_DOUBLE, &ddt[Y]);
    MPI_Type_commit(&ddt[Y]);
    MPI_Type_vector(delems[DY] - 2, delems[DX] - 2, delems[DX], MPI_DOUBLE, &ddt[Z]);
    MPI_Type_commit(&ddt[Z]);
}

static void populate(double *data, size_t amount, double *contig_buf, size_t contig_count)
{
    size_t i;
    for (i = 0; i < amount; i++)
        data[i] = (rank + 1) * i;

    for (i = 0; i < contig_count; i++)
        contig_buf[i] = (rank + 1) * i;
}

#ifdef DEBUG
static void print_cube(double *dd)
{
    int x, y, z, i = 0;

    fprintf(stderr, "[%d] in print_cube, dd=%p\n", rank, dd);

    for (z = 0; z < delems[DZ]; z++) {
        for (y = 0; y < delems[DY]; y++) {
            for (x = 0; x < delems[DX]; x++) {
                fprintf(stderr, "%2.1f ", dd[i++]);
            }
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
    }
}
#endif

#ifdef CHECK
static inline int check_elm_val(size_t off, double elm_val, double val)
{
    if ((int) (elm_val - val) != 0) {
        fprintf(stderr, "[%d] dst[off] %ld %.1f != val %.1f\n",
                rank, off, elm_val, val);
        fflush(stderr);

        return 1;
    }
    return 0;
}

static int check_results(void)
{
    int z, y, x, src_rank;
    size_t init_val = 0, off = 0, coff = 0;
    int err = 0;
#ifdef CONTIG_TO_VEC
    int rd = L;
#else
    int sd = R;
#endif

    src_rank = (rank == 0) ? 1 : 0;

#ifdef CONTIG_TO_VEC
    init_val = 0;
#else
    /* get the start offset in send matrix */
    init_val = src[test_dim][sd] - data;
#endif

    switch (test_dim) {
    case X:
        /* send out right plane and receive left plane */
        for (z = 0; z < delems[DZ] - 2; z++) {
            for (y = 0; y < delems[DY] - 2; y++) {
                off = z * delems[DX] * delems[DY] + y * delems[DX];
#ifdef CONTIG_TO_VEC
                err = check_elm_val((dst[test_dim][rd] - data + off),
                                    *(dst[test_dim][rd] + off) /* dest */ ,
                                    (src_rank + 1) * (init_val + coff) /* src_value */);
#else
                /* default VEC_TO_CONTIG */
                err = check_elm_val(coff, contig_buf[coff] /* dest */ ,
                                    (src_rank + 1) * (init_val + off) /* src_value */);
#endif
                if (err > 0)
                    goto exit;

                coff++;
            }
        }
        break;

    case Y:
        /* send out bottom plane and receive top plane */
        for (z = 0; z < delems[DZ] - 2; z++) {
            for (x = 0; x < delems[DX] - 2; x++) {
                off = z * delems[DX] * delems[DY] + x;
#ifdef CONTIG_TO_VEC
                err = check_elm_val((dst[test_dim][rd] - data + off),
                                    *(dst[test_dim][rd] + off) /* dest */ ,
                                    (src_rank + 1) * (init_val + coff) /* src_value */);
#else
                /* default VEC_TO_CONTIG */
                err = check_elm_val(coff, contig_buf[coff] /* dest */ ,
                                    (src_rank + 1) * (init_val + off) /* src_value */);
#endif
                if (err > 0)
                    goto exit;

                coff++;
            }
        }
        break;

    case Z:
        /* send out back plane and receive front plane */
        for (y = 0; y < delems[DY] - 2; y++) {
            for (x = 0; x < delems[DX] - 2; x++) {
                off = y * delems[DX] + x;
#ifdef CONTIG_TO_VEC
                err = check_elm_val((dst[test_dim][rd] - data + off),
                                    *(dst[test_dim][rd] + off) /* dest */ ,
                                    (src_rank + 1) * (init_val + coff) /* src_value */);
#else
                /* default VEC_TO_CONTIG */
                err = check_elm_val(coff, contig_buf[coff] /* dest */ ,
                                    (src_rank + 1) * (init_val + off) /* src_value */);
#endif
                if (err > 0)
                    goto exit;

                coff++;
            }
        }
        break;
    }

  exit:
    return err;
}

#endif

static void finish(const char format[], ...)
{
    va_list list;

    if (rank == 0) {
        va_start(list, format);
        vfprintf(stderr, format, list);
        va_end(list);
        fflush(stderr);
    }

    MPI_Finalize();
    exit(1);
}

static int run_iter(void)
{
#ifndef SINGLE_EXCHANGE
    MPI_Request reqs[2];
#endif
    MPI_Status stats[2];
    double t0, t1, t2, agv_t;
    int i, dst_rank;
#ifdef CONTIG_TO_VEC
    int rd = L;
#else
    int sd = R;
#endif
    int err = 0;
    double *sbuf = NULL, *rbuf = NULL;
    int sbuf_count = 0, rbuf_count = 0;
    MPI_Aint ddt_extent = 0;
    int ddt_size = 0;
    MPI_Datatype sdatatype, rdatatype;

    agv_t = 0.0;
    MPI_Barrier(MPI_COMM_WORLD);
    t0 = MPI_Wtime();

    dst_rank = (rank == 0) ? 1 : 0;

#ifdef CONTIG_TO_VEC
    rbuf = dst[test_dim][rd];
    rbuf_count = 1;
    rdatatype = ddt[test_dim];
    sbuf = contig_buf;
    sbuf_count = contig_count;
    sdatatype = MPI_DOUBLE;
#else
    /* default VEC_TO_CONTIG */
    sbuf = src[test_dim][sd];
    sbuf_count = 1;
    sdatatype = ddt[test_dim];
    rbuf = contig_buf;
    rbuf_count = contig_count;
    rdatatype = MPI_DOUBLE;
#endif

    MPI_Type_extent(ddt[test_dim], &ddt_extent);
    MPI_Type_size(ddt[test_dim], &ddt_size);

#ifdef USE_PAPI
    PAPI_START();
#endif

#ifdef SINGLE_EXCHANGE
    /* Only rank 1 sends plane */
    if (rank == 1) {
        for (i = 0; i < reps; i++) {
            MPI_Send(sbuf, sbuf_count, sdatatype, dst_rank, 0, MPI_COMM_WORLD);
        }
    }
    else {
        for (i = 0; i < reps; i++) {
            MPI_Recv(rbuf, rbuf_count, rdatatype, dst_rank, 0, MPI_COMM_WORLD, &stats[0]);
        }
    }
#else
    for (i = 0; i < reps; i++) {
        MPI_Irecv(rbuf, rbuf_count, rdatatype, dst_rank, 0, MPI_COMM_WORLD, &reqs[0]);
        MPI_Isend(sbuf, sbuf_count, sdatatype, dst_rank, 0, MPI_COMM_WORLD, &reqs[1]);

        MPI_Waitall(2, reqs, stats);
    }
#endif

#ifdef USE_PAPI
    PAPI_STOP();
#endif

    t1 = MPI_Wtime() - t0;
    MPI_Reduce(&t1, &t2, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    agv_t = t2 / size;

#ifdef CHECK
#ifdef SINGLE_EXCHANGE
    if (rank == 0)
        err = check_results();
#else
    err = check_results();
#endif
    MPI_Barrier(MPI_COMM_WORLD);
#endif

#ifdef USE_PAPI
    /* rank 0 gathers all papi data, do not average ! */
    if (rank == 1) {
        MPI_Send(&papi_values[1], 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Recv(&papi_values[1], 1, MPI_LONG_LONG, 1, 0, MPI_COMM_WORLD, &stats[0]);
    }
#endif

    if (rank == 0) {
        agv_t /= reps;
        fprintf(stdout, "%s iter %d, x %d, y %d, z %d, test_dim %s, time %.4lf"
#ifdef USE_PAPI
                ", %s %lld %lld"
#endif
                ", size-k %d, extent-k %ld\n",
                test_name, reps, delems[DX], delems[DY], delems[DZ], dim_str[test_dim],
                agv_t * 1000 * 1000
#ifdef USE_PAPI
                , event_name, papi_values[0] / reps, papi_values[1] / reps
#endif
                ,ddt_size/1024, ddt_extent/1024);
        fflush(stdout);
    }
    return err;
}

int main(int argc, char **argv)
{
    int i, err = 0;
    size_t plane = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#ifdef BATCH
    reps = 100;

    delems[DX] = DELEM_X;
    delems[DY] = DELEM_Y;
    delems[DZ] = DELEM_Z;
    test_dim = TEST_DIM;        /* X=0 (l,r), Y=1 (t,b), Z=2 (f,b) */
#else
    if (argc < 6) {
        finish
            ("Usage: <number of repetitions> <X doubles> <Y doubles> <Z doubles> <test dim: 0=X (l,r) | 1=Y (t,b) | 2=Z (f,b)>\n");
    }

    reps = atoi(argv[1]);
    delems[DX] = atoi(argv[2]);
    delems[DY] = atoi(argv[3]);
    delems[DZ] = atoi(argv[4]);
    test_dim = atoi(argv[5]);
#endif

#ifdef USE_PAPI
    {
        event_code = event_codes[0];
        if (argc >= 7) {
            int event_id = atoi(argv[6]);
            if (event_id >= 0 && event_id < PAPI_EVENT_MAX)
                event_code = event_codes[event_id];
        }
        if ((err = papi_init()))
            MPI_Abort(MPI_COMM_WORLD, -1);
    }
#endif

    if (delems[DX] <= 0 || delems[DY] <= 0 || delems[DZ] < 0) {
        finish("Error parsing <X,Y,Z> dimension elements: %d,%d,%d.",
               delems[DX], delems[DY], delems[DZ]);
    }

    if (dims <= test_dim) {
        finish("Invalid test dimension %d with dimension elements <%d,%d,%d>.",
               test_dim, delems[DX], delems[DY], delems[DZ]);
    }

    allocate(&data, (size_t) delems[DX] * delems[DY] * delems[DZ]);
    fillddt(ddt, delems);
    prepare_contig(&contig_buf, test_dim);

    plane = delems[DX] * delems[DY];
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

    populate(data, delems[DX] * delems[DY] * delems[DZ], contig_buf, contig_count);

#ifdef DEBUG
    {
        int r;
        for (r = 0; r < size; r++) {
            if (r == rank)
                print_cube(data);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
#endif

    err = run_iter();

#ifdef DEBUG
    {
        if (rank == 0) {
            fprintf(stdout, "after exchange ----\n");
            fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        int r;
        for (r = 0; r < size; r++) {
            if (r == rank)
                print_cube(data);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
#endif

#ifdef CHECK
    if (err == 0 && rank == 0) {
        fprintf(stdout, "No Error \n");
        fflush(stdout);
    }
#endif

    for (i = 0; i < dims; i++)
        MPI_Type_free(&ddt[i]);
    MPI_Type_free(&ddt_col);

    MPI_Finalize();

#ifdef USE_PAPI
    if ((err = papi_exit()))
        MPI_Abort(MPI_COMM_WORLD, -1);
#endif

    if (data)
        free(data);
    if (contig_buf)
        free(contig_buf);

    return 0;
}
