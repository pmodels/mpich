/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock+flush";
*/

typedef struct {
    const char *typename;
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

int main(int argc, char *argv[])
{
    int errs = 0;
    int err = 0;
    int rank, size, orig, target;
    int minsize = 2, count;
    int i, j;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint lb, extent;
    MPI_Datatype origtype, targettype;
    DTP_t orig_dtp, target_dtp;
    void *origbuf, *targetbuf;

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;
    int len;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };

    /* TODO: parse input parameters using optarg */
    if (argc < 3) {
        fprintf(stdout, "Usage: %s -type=[TYPE] -count=[COUNT]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-type=", strlen("-type="))) {
                j = 0;
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
#else
    MPI_Datatype *basic_types = NULL;
    int *basic_type_counts = NULL;
    int basic_type_num;
    int k;
    char *input_string, *token;

    /* TODO: parse input parameters using optarg */
    if (argc < 4) {
        fprintf(stdout, "Usage: %s -numtypes=[NUM] -types=[TYPES] -counts=[COUNTS]\n", argv[0]);
        return MTestReturnValue(1);
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-numtypes=", strlen("-numtypes="))) {
                basic_type_num = atoi(argv[i] + strlen("-numtypes="));

                /* allocate arrays */
                basic_type_counts = (int *) malloc(basic_type_num * sizeof(int));
                basic_types = (MPI_Datatype *) malloc(basic_type_num * sizeof(MPI_Datatype));
            } else if (!strncmp(argv[i], "-types=", strlen("-type="))) {
                input_string = strdup(argv[i] + strlen("-types="));
                for (k = 0, token = strtok(input_string, ","); token; token = strtok(NULL, ",")) {
                    j = 0;
                    while (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL") &&
                           strcmp(token, typelist[j].typename)) {
                        j++;
                    }

                    if (strcmp(typelist[j].typename, "MPI_DATATYPE_NULL")) {
                        basic_types[k++] = typelist[j].type;
                    } else {
                        fprintf(stdout, "Error: datatype not recognized\n");
                        return MTestReturnValue(1);
                    }
                }
                free(input_string);
            } else if (!strncmp(argv[i], "-counts=", strlen("-counts="))) {
                input_string = strdup(argv[i] + strlen("-counts="));
                for (k = 0, token = strtok(input_string, ","); token; token = strtok(NULL, ",")) {
                    basic_type_counts[k++] = atoi(token);
                }
                free(input_string);
            }
        }
    }

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &orig_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &target_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    /* this is ignored */
    count = 0;
#endif

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig = 0;
        target = size - 1;

        for (i = 0; i < target_dtp->DTP_num_objs; i++) {
            err = DTP_obj_create(target_dtp, i, 0, 0, 0);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            targetcount = target_dtp->DTP_obj_array[i].DTP_obj_count;
            targettype = target_dtp->DTP_obj_array[i].DTP_obj_type;
            targetbuf = target_dtp->DTP_obj_array[i].DTP_obj_buf;

            MPI_Type_get_extent(targettype, &lb, &extent);
            MPI_Win_create(targetbuf, lb + targetcount * extent,
                           (int) extent, MPI_INFO_NULL, comm, &win);

            for (j = 0; j < orig_dtp->DTP_num_objs; j++) {
                err = DTP_obj_create(orig_dtp, j, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                origcount = orig_dtp->DTP_obj_array[j].DTP_obj_count;
                origtype = orig_dtp->DTP_obj_array[j].DTP_obj_type;
                origbuf = orig_dtp->DTP_obj_array[j].DTP_obj_buf;

                if (rank == orig) {
                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    MPI_Accumulate(origbuf, origcount,
                                   origtype, target, 0, targetcount, targettype, MPI_REPLACE, win);
                    MPI_Win_flush(target, win);
                    /*signal to target that the ops are flushed so that it starts checking the result */
                    MPI_Barrier(comm);
                    /*make sure target finishes checking the result before issuing unlock */
                    MPI_Barrier(comm);
                    MPI_Win_unlock(target, win);

                    /*wait for the destination to finish checking and reinitializing the buffer */
                    MPI_Barrier(comm);

                    char *resbuf = (char *) calloc(lb + extent * targetcount, sizeof(char));

                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    MPI_Get_accumulate(origbuf, origcount,
                                       origtype, resbuf, targetcount, targettype,
                                       target, 0, targetcount, targettype, MPI_REPLACE, win);
                    MPI_Win_flush(target, win);
                    /*signal to target that the ops are flushed so that it starts checking the result */
                    MPI_Barrier(comm);
                    /*make sure target finishes checking the result before issuing unlock */
                    MPI_Barrier(comm);
                    MPI_Win_unlock(target, win);
                    free(resbuf);
                } else if (rank == target) {
                    /* TODO: add a DTP_buf_set() function to replace this */
                    char *tmp = (char *) calloc(lb + extent * targetcount, sizeof(char));
                    memcpy(tmp, targetbuf, lb + extent * targetcount);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                    }
                    /* restore target buffer */
                    memcpy(targetbuf, tmp, lb + extent * targetcount);
                    free(tmp);

                    MPI_Barrier(comm);
                    MPI_Win_unlock(target, win);

                    /*signal the source that checking and reinitialization is done */
                    MPI_Barrier(comm);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                    }
                    MPI_Barrier(comm);
                    MPI_Win_unlock(target, win);
                }
                DTP_obj_free(orig_dtp, j);
            }
            MPI_Win_free(&win);
            DTP_obj_free(target_dtp, i);
        }
        MTestFreeComm(&comm);
    }

    DTP_pool_free(orig_dtp);
    DTP_pool_free(target_dtp);

#ifdef USE_DTP_POOL_TYPE__STRUCT
    /* cleanup array if any */
    if (basic_types) {
        free(basic_types);
    }
    if (basic_type_counts) {
        free(basic_type_counts);
    }
#endif

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
