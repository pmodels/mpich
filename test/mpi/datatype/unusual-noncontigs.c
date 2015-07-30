/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

/*
   The default behavior of the test routines should be to briefly indicate
   the cause of any errors - in this test, that means that verbose needs
   to be set. Verbose should turn on output that is independent of error
   levels.
*/
static int verbose = 1;

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);
int struct_negdisp_test(void);
int vector_negstride_test(void);
int indexed_negdisp_test(void);
int struct_struct_test(void);
int flatten_test(void);

int build_array_section_type(MPI_Aint aext, MPI_Aint astart, MPI_Aint aend,
                             MPI_Datatype * datatype);

int main(int argc, char *argv[])
{
    int err, errs = 0;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    err = struct_negdisp_test();
    if (verbose && err)
        fprintf(stderr, "error in struct_negdisp_test\n");
    errs += err;

    err = vector_negstride_test();
    if (verbose && err)
        fprintf(stderr, "error in vector_negstride_test\n");
    errs += err;

    err = indexed_negdisp_test();
    if (verbose && err)
        fprintf(stderr, "error in indexed_negdisp_test\n");
    errs += err;

    err = struct_struct_test();
    if (verbose && err)
        fprintf(stderr, "error in struct_struct_test\n");
    errs += err;

    err = flatten_test();
    if (verbose && err)
        fprintf(stderr, "error in flatten_test\n");
    errs += err;

    /* print message and exit */
    if (errs) {
        fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* test uses a struct type that describes data that is contiguous,
 * but processed in a noncontiguous way.
 */
int struct_negdisp_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype mystruct;
    MPI_Request request;
    MPI_Status status;

    MPI_Aint disps[2] = { 0, -1 * ((int) sizeof(int)) };
    int blks[2] = { 1, 1, };
    MPI_Datatype types[2] = { MPI_INT, MPI_INT };

    err = MPI_Type_struct(2, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Type_struct returned error\n");
        }
    }

    MPI_Type_commit(&mystruct);

    err = MPI_Irecv(recvbuf + 1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Irecv returned error\n");
        }
    }

    err = MPI_Send(sendbuf + 2, 2, mystruct, 0, 0, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Send returned error\n");
        }
    }

    err = MPI_Wait(&request, &status);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Wait returned error\n");
        }
    }

    /* verify data */
    if (recvbuf[0] != -1) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
        }
    }
    if (recvbuf[1] != 3) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
        }
    }
    if (recvbuf[2] != 2) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
        }
    }
    if (recvbuf[3] != 5) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
        }
    }
    if (recvbuf[4] != 4) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
        }
    }
    if (recvbuf[5] != -6) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
        }
    }

    MPI_Type_free(&mystruct);

    return errs;
}

/* test uses a vector type that describes data that is contiguous,
 * but processed in a noncontiguous way.  this is effectively the
 * same type as in the struct_negdisp_test above.
 */
int vector_negstride_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype myvector;
    MPI_Request request;
    MPI_Status status;

    err = MPI_Type_vector(2, 1, -1, MPI_INT, &myvector);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Type_vector returned error\n");
        }
    }

    MPI_Type_commit(&myvector);

    err = MPI_Irecv(recvbuf + 1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Irecv returned error\n");
        }
    }

    err = MPI_Send(sendbuf + 2, 2, myvector, 0, 0, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Send returned error\n");
        }
    }

    err = MPI_Wait(&request, &status);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Wait returned error\n");
        }
    }

    /* verify data */
    if (recvbuf[0] != -1) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
        }
    }
    if (recvbuf[1] != 3) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
        }
    }
    if (recvbuf[2] != 2) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
        }
    }
    if (recvbuf[3] != 5) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
        }
    }
    if (recvbuf[4] != 4) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
        }
    }
    if (recvbuf[5] != -6) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
        }
    }

    MPI_Type_free(&myvector);

    return errs;
}

/* test uses a indexed type that describes data that is contiguous,
 * but processed in a noncontiguous way.  this is effectively the same
 * type as in the two tests above.
 */
int indexed_negdisp_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype myindexed;
    MPI_Request request;
    MPI_Status status;

    int disps[2] = { 0, -1 };
    int blks[2] = { 1, 1 };

    err = MPI_Type_indexed(2, blks, disps, MPI_INT, &myindexed);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Type_indexed returned error\n");
        }
    }

    MPI_Type_commit(&myindexed);

    err = MPI_Irecv(recvbuf + 1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Irecv returned error\n");
        }
    }

    err = MPI_Send(sendbuf + 2, 2, myindexed, 0, 0, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Send returned error\n");
        }
    }

    err = MPI_Wait(&request, &status);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Wait returned error\n");
        }
    }

    /* verify data */
    if (recvbuf[0] != -1) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
        }
    }
    if (recvbuf[1] != 3) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
        }
    }
    if (recvbuf[2] != 2) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
        }
    }
    if (recvbuf[3] != 5) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
        }
    }
    if (recvbuf[4] != 4) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
        }
    }
    if (recvbuf[5] != -6) {
        errs++;
        if (verbose) {
            fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
        }
    }

    MPI_Type_free(&myindexed);

    return errs;
}

#define check_err(fn_name_)                                                   \
    do {                                                                      \
        if (err != MPI_SUCCESS) {                                             \
            errs++;                                                           \
            if (verbose) {                                                    \
                int len_;                                                     \
                char err_str_[MPI_MAX_ERROR_STRING];                          \
                MPI_Error_string(err, err_str_, &len_);                       \
                fprintf(stderr, #fn_name_ " failed at line %d, err=%d: %s\n", \
                        __LINE__, err, err_str_);                             \
            }                                                                 \
        }                                                                     \
    } while (0)
/* test case from tt#1030 ported to C
 *
 * Thanks to Matthias Lieber for reporting the bug and providing a good test
 * program. */
int struct_struct_test(void)
{
    int err, errs = 0;
    int i, j, dt_size = 0;
    MPI_Request req[2];


#define COUNT (2)
    MPI_Aint displ[COUNT];
    int blens[COUNT];
    MPI_Datatype types[COUNT];
    MPI_Datatype datatype;

    /* A slight difference from the F90 test: F90 arrays are column-major, C
     * arrays are row-major.  So we invert the order of dimensions. */
#define N (2)
#define M (4)
    int array[N][M] = { {-1, -1, -1, -1}, {-1, -1, -1, -1} };
    int expected[N][M] = { {-1, 1, 2, 5}, {-1, 3, 4, 6} };
    int seq_array[N * M];
    MPI_Aint astart, aend;
    MPI_Aint size_exp = 0;

    /* 1st section selects elements 1 and 2 out of 2nd dimension, complete 1st dim.
     * should receive the values 1, 2, 3, 4 */
    astart = 1;
    aend = 2;
    err = build_array_section_type(M, astart, aend, &types[0]);
    if (err) {
        errs++;
        if (verbose)
            fprintf(stderr, "build_array_section_type failed\n");
        return errs;
    }
    blens[0] = N;
    displ[0] = 0;
    size_exp = size_exp + N * (aend - astart + 1) * sizeof(int);

    /* 2nd section selects last element of 2nd dimension, complete 1st dim.
     * should receive the values 5, 6 */
    astart = 3;
    aend = 3;
    err = build_array_section_type(M, astart, aend, &types[1]);
    if (err) {
        errs++;
        if (verbose)
            fprintf(stderr, "build_array_section_type failed\n");
        return errs;
    }
    blens[1] = N;
    displ[1] = 0;
    size_exp = size_exp + N * (aend - astart + 1) * sizeof(int);

    /* create type */
    err = MPI_Type_create_struct(COUNT, blens, displ, types, &datatype);
    check_err(MPI_Type_create_struct);
    err = MPI_Type_commit(&datatype);
    check_err(MPI_Type_commit);

    err = MPI_Type_size(datatype, &dt_size);
    check_err(MPI_Type_size);
    if (dt_size != size_exp) {
        errs++;
        if (verbose)
            fprintf(stderr, "unexpected type size\n");
    }


    /* send the type to ourselves to make sure that the type describes data correctly */
    for (i = 0; i < (N * M); ++i)
        seq_array[i] = i + 1;   /* source values 1..(N*M) */
    err = MPI_Isend(&seq_array[0], dt_size / sizeof(int), MPI_INT, 0, 42, MPI_COMM_SELF, &req[0]);
    check_err(MPI_Isend);
    err = MPI_Irecv(&array[0][0], 1, datatype, 0, 42, MPI_COMM_SELF, &req[1]);
    check_err(MPI_Irecv);
    err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
    check_err(MPI_Waitall);

    /* check against expected */
    for (i = 0; i < N; ++i) {
        for (j = 0; j < M; ++j) {
            if (array[i][j] != expected[i][j]) {
                errs++;
                if (verbose)
                    fprintf(stderr, "array[%d][%d]=%d, should be %d\n", i, j, array[i][j],
                            expected[i][j]);
            }
        }
    }

    err = MPI_Type_free(&datatype);
    check_err(MPI_Type_free);
    err = MPI_Type_free(&types[0]);
    check_err(MPI_Type_free);
    err = MPI_Type_free(&types[1]);
    check_err(MPI_Type_free);

    return errs;
#undef M
#undef N
#undef COUNT
}

/*   create a datatype for a 1D int array subsection

     - a subsection of the first dimension is defined via astart, aend
     - indexes are assumed to start with 0, that means:
       - 0 <= astart <= aend < aext
     - astart and aend are inclusive

     example:

     aext = 8, astart=2, aend=4 would produce:

     index     | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
     1D array   ###############################
     datatype   LB      ###########             UB
 */
int build_array_section_type(MPI_Aint aext, MPI_Aint astart, MPI_Aint aend, MPI_Datatype * datatype)
{
#define COUNT (3)
    int err, errs = 0;
    MPI_Aint displ[COUNT];
    int blens[COUNT];
    MPI_Datatype types[COUNT];

    *datatype = MPI_DATATYPE_NULL;

    /* lower bound marker */
    types[0] = MPI_LB;
    displ[0] = 0;
    blens[0] = 1;

    /* subsection starting at astart */
    displ[1] = astart * sizeof(int);
    types[1] = MPI_INT;
    blens[1] = aend - astart + 1;

    /* upper bound marker */
    types[2] = MPI_UB;
    displ[2] = aext * sizeof(int);
    blens[2] = 1;

    err = MPI_Type_create_struct(COUNT, blens, displ, types, datatype);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "MPI_Type_create_struct failed, err=%d\n", err);
        }
    }

    return errs;
#undef COUNT
}

/* start_idx is the "zero" point for the unpack */
static int pack_and_check_expected(MPI_Datatype type, const char *name,
                                   int start_idx, int size, int *array, int *expected)
{
    int i;
    int err, errs = 0;
    int pack_size = -1;
    int *pack_buf = NULL;
    int pos;
    int type_size = -1;
    int sendbuf[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    err = MPI_Type_size(type, &type_size);
    check_err(MPI_Type_size);
    assert(sizeof(sendbuf) >= type_size);

    err = MPI_Pack_size(type_size / sizeof(int), MPI_INT, MPI_COMM_SELF, &pack_size);
    check_err(MPI_Pack_size);
    pack_buf = malloc(pack_size);
    assert(pack_buf);

    pos = 0;
    err =
        MPI_Pack(&sendbuf[0], type_size / sizeof(int), MPI_INT, pack_buf, pack_size, &pos,
                 MPI_COMM_SELF);
    check_err(MPI_Pack);
    pos = 0;
    err = MPI_Unpack(pack_buf, pack_size, &pos, &array[start_idx], 1, type, MPI_COMM_SELF);
    check_err(MPI_Unpack);
    free(pack_buf);

    /* check against expected */
    for (i = 0; i < size; ++i) {
        if (array[i] != expected[i]) {
            errs++;
            if (verbose)
                fprintf(stderr, "%s: array[%d]=%d, should be %d\n", name, i, array[i], expected[i]);
        }
    }

    return errs;
}

/* regression for tt#1030, checks for bad offset math in the
 * blockindexed and indexed dataloop flattening code */
int flatten_test(void)
{
    int err, errs = 0;
#define ARR_SIZE (9)
    /* real indices              0  1  2  3  4  5  6  7  8
     * indices w/ &array[3]     -3 -2 -1  0  1  2  3  4  5 */
    int array[ARR_SIZE] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    int expected[ARR_SIZE] = { -1, 0, 1, -1, 2, -1, 3, -1, 4 };
    MPI_Datatype idx_type = MPI_DATATYPE_NULL;
    MPI_Datatype blkidx_type = MPI_DATATYPE_NULL;
    MPI_Datatype combo = MPI_DATATYPE_NULL;
#define COUNT (2)
    int displ[COUNT];
    MPI_Aint adispl[COUNT];
    int blens[COUNT];
    MPI_Datatype types[COUNT];

    /* indexed type layout:
     * XX_X
     * 2101  <-- pos (left of 0 is neg)
     *
     * different blens to prevent optimization into a blockindexed
     */
    blens[0] = 2;
    displ[0] = -2;      /* elements, puts byte after block end at 0 */
    blens[1] = 1;
    displ[1] = 1;       /*elements */

    err = MPI_Type_indexed(COUNT, blens, displ, MPI_INT, &idx_type);
    check_err(MPI_Type_indexed);
    err = MPI_Type_commit(&idx_type);
    check_err(MPI_Type_commit);

    /* indexed type layout:
     * _X_X
     * 2101  <-- pos (left of 0 is neg)
     */
    displ[0] = -1;
    displ[1] = 1;
    err = MPI_Type_create_indexed_block(COUNT, 1, displ, MPI_INT, &blkidx_type);
    check_err(MPI_Type_indexed_block);
    err = MPI_Type_commit(&blkidx_type);
    check_err(MPI_Type_commit);

    /* struct type layout:
     * II_I_B_B  (I=idx_type, B=blkidx_type)
     * 21012345  <-- pos (left of 0 is neg)
     */
    blens[0] = 1;
    adispl[0] = 0;      /*bytes */
    types[0] = idx_type;

    blens[1] = 1;
    adispl[1] = 4 * sizeof(int);        /* bytes */
    types[1] = blkidx_type;

    /* must be a struct in order to trigger flattening code */
    err = MPI_Type_create_struct(COUNT, blens, adispl, types, &combo);
    check_err(MPI_Type_indexed);
    err = MPI_Type_commit(&combo);
    check_err(MPI_Type_commit);

    /* pack/unpack with &array[3] */
    errs += pack_and_check_expected(combo, "combo", 3, ARR_SIZE, array, expected);

    MPI_Type_free(&combo);
    MPI_Type_free(&idx_type);
    MPI_Type_free(&blkidx_type);

    return errs;
#undef COUNT
}

#undef check_err

int parse_args(int argc, char **argv)
{
    /*
     * int ret;
     *
     * while ((ret = getopt(argc, argv, "v")) >= 0)
     * {
     * switch (ret) {
     * case 'v':
     * verbose = 1;
     * break;
     * }
     * }
     */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    return 0;
}
