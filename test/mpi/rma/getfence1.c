/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

#define MAX_COUNT_SIZE (16000000)
#define MAX_TYPE_SIZE  (16)

typedef struct {
    char *typename;
    MPI_Datatype type;
} Type_t;

Type_t typelist[] = {
    {"MPI_CHAR", MPI_CHAR},
    {"MPI_BYTE", MPI_BYTE},
    {"MPI_WCHAR", MPI_WCHAR},
    {"MPI_SHORT", MPI_SHORT},
    {"MPI_INT", MPI_INT},
    {"MPI_LONG", MPI_LONG},
    {"MPI_LONG_LONG_INT", MPI_LONG_LONG_INT},
    {"MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR},
    {"MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT},
    {"MPI_UNSIGNED", MPI_UNSIGNED},
    {"MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG},
    {"MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG},
    {"MPI_FLOAT", MPI_FLOAT},
    {"MPI_DOUBLE", MPI_DOUBLE},
    {"MPI_LONG_DOUBLE", MPI_LONG_DOUBLE},
    {"MPI_INT8_T", MPI_INT8_T},
    {"MPI_INT16_T", MPI_INT16_T},
    {"MPI_INT32_T", MPI_INT32_T},
    {"MPI_INT64_T", MPI_INT64_T},
    {"MPI_UINT8_T", MPI_UINT8_T},
    {"MPI_UINT16_T", MPI_UINT16_T},
    {"MPI_UINT32_T", MPI_UINT32_T},
    {"MPI_UINT64_T", MPI_UINT64_T},
    {"MPI_C_COMPLEX", MPI_C_COMPLEX},
    {"MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX},
    {"MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX},
    {"MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX},
    {"MPI_FLOAT_INT", MPI_FLOAT_INT},
    {"MPI_DOUBLE_INT", MPI_DOUBLE_INT},
    {"MPI_LONG_INT", MPI_LONG_INT},
    {"MPI_2INT", MPI_2INT},
    {"MPI_SHORT_INT", MPI_SHORT_INT},
    {"MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT},
    {"MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}
};

static inline int test(MPI_Comm comm, int rank, int orig, int target,
                       int i, int j, DTP_t orig_dtp, DTP_t target_dtp)
{
    int errs = 0, err;
    int disp_unit;
    int len;
    MPI_Aint origcount, targetcount, count;
    MPI_Aint extent, lb;
    MPI_Win win;
    MPI_Datatype origtype, targettype;
    char orig_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char target_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *origbuf, *targetbuf;

    count = orig_dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;
    origtype = orig_dtp->DTP_obj_array[i].DTP_obj_type;
    targettype = target_dtp->DTP_obj_array[j].DTP_obj_type;
    origcount = orig_dtp->DTP_obj_array[i].DTP_obj_count;
    targetcount = target_dtp->DTP_obj_array[j].DTP_obj_count;
    origbuf = orig_dtp->DTP_obj_array[i].DTP_obj_buf;
    targetbuf = target_dtp->DTP_obj_array[j].DTP_obj_buf;

    MPI_Type_get_name(origtype, orig_name, &len);
    MPI_Type_get_name(targettype, target_name, &len);

    MTestPrintfMsg(1,
                   "Getting count = %ld of origtype %s - count = %ld receive type %s\n",
                   origcount, orig_name, targetcount, target_name);

    MPI_Type_get_extent(origtype, &lb, &extent);
    /* This makes sure that disp_unit does not become negative for large counts */
    disp_unit = extent < INT_MAX ? extent : 1;
    MPI_Win_create(origbuf, origcount * extent + lb, disp_unit, MPI_INFO_NULL, comm, &win);
    MPI_Win_fence(0, win);

    if (rank == orig) {
        /* The orig does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
    } else if (rank == target) {
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        /* This should have the same effect, in terms of
         * transfering data, as a send/recv pair */
        err = MPI_Get(targetbuf, targetcount, targettype, orig, 0, origcount, origtype, win);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        err = MPI_Win_fence(0, win);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        err = DTP_obj_buf_check(target_dtp, j, 0, 1, count);
        if (err != DTP_SUCCESS) {
            errs++;
            if (errs < 10) {
                printf
                    ("Data in target buffer did not match for targetination datatype %s (put with orig datatype %s)\n",
                     target_name, orig_name);
            }
        }
    } else {
        MPI_Win_fence(0, win);
    }
    MPI_Win_free(&win);

    return errs;
}


int main(int argc, char *argv[])
{
    int err, errs = 0;
    int rank, size, orig, target;
    int minsize = 2;
    int i, j;
    int count;
    int len;
    MPI_Aint bufsize;
    MPI_Aint lb, extent;
    MPI_Comm comm;
    MPI_Datatype basic_type;
    DTP_t orig_dtp, target_dtp;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };

    MTest_Init(&argc, &argv);

    /* TODO: parse input parameters using optarg */
    if (argc < 3) {
        fprintf(stdout, "Usage: %s -type=[TYPE] -count=[COUNT]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-type=", strlen("-type="))) {
                while (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL") &&
                       strcmp(argv[i] + strlen("-type="), typelist[j].typename)) {
                    j++;
                }

                if (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL")) {
                    basic_type = typelist[j].type;
                } else {
                    fprintf(stdout, "Error: datatype not recognized\n");
                    return MTestReturnValue(1);
                }
            } else if (!strncmp(argv[i], "-count=", strlen("-count="))) {
                count = atoi(argv[i] + strlen("-count="));
                /* TODO: make sure count is valid */
            }
        }
    }

    /* compute bufsize to limit number of comm in test */
    MPI_Type_get_extent(basic_type, &lb, &extent);
    bufsize = extent * count;

    err = DTP_pool_create(basic_type, count, &orig_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating orig pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }

    err = DTP_pool_create(basic_type, count, &target_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating target pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig = 0;
        target = size - 1;

        for (i = 0; i < orig_dtp->DTP_num_objs; i++) {
            err = DTP_obj_create(orig_dtp, i, 0, 1, count);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            for (j = 0; j < target_dtp->DTP_num_objs; j++) {
                err = DTP_obj_create(target_dtp, j, 0, 0, 0);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                errs += test(comm, rank, orig, target, i, j, orig_dtp, target_dtp);

                DTP_obj_free(target_dtp, j);
            }
            DTP_obj_free(orig_dtp, i);
        }
        MTestFreeComm(&comm);

        /* for large buffers only do one communicator */
        if (MAX_COUNT_SIZE * MAX_TYPE_SIZE < bufsize) {
            break;
        }
    }

    DTP_pool_free(orig_dtp);
    DTP_pool_free(target_dtp);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
