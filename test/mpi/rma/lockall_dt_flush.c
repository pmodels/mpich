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
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock_all+flush";
*/

#define MAX_COUNT_SIZE (262144)
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

int main(int argc, char *argv[])
{
    int err, errs = 0;
    int rank, size;
    int minsize = 2, count;
    int len;
    int i, j;
    MPI_Aint origcount, targetcount;
    MPI_Aint bufsize;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint lb, extent;
    MPI_Datatype origtype, targettype;
    MPI_Datatype basic_type;
    DTP_t orig_dtp, target_dtp;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *origbuf, *targetbuf;

    MTest_Init(&argc, &argv);

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

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        int orig = 0;

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
                    int target;
                    MPI_Win_lock_all(0, win);

                    for (target = 0; target < size; target++)
                        if (target != orig) {
                            MPI_Accumulate(origbuf, origcount,
                                           origtype, target, 0,
                                           targetcount, targettype, MPI_REPLACE, win);
                            MPI_Win_flush(target, win);
                        }
                    /*signal to target that the ops are flushed so that it starts checking the result */
                    MPI_Barrier(comm);
                    /*make sure target finishes checking the result before issuing unlock */
                    MPI_Barrier(comm);
                    MPI_Win_unlock_all(win);

                    char *resbuf = (char *) calloc(lb + extent * targetcount, sizeof(char));

                    /*wait for the destination to finish checking and reinitializing the buffer */
                    MPI_Barrier(comm);

                    MPI_Win_lock_all(0, win);
                    for (target = 0; target < size; target++)
                        if (target != orig) {
                            MPI_Get_accumulate(origbuf, origcount,
                                               origtype, resbuf, targetcount,
                                               targettype, target, 0, targetcount,
                                               targettype, MPI_REPLACE, win);
                            MPI_Win_flush(target, win);
                        }
                    /*signal to target that the ops are flushed so that it starts checking the result */
                    MPI_Barrier(comm);
                    /*make sure target finishes checking the result before issuing unlock */
                    MPI_Barrier(comm);
                    MPI_Win_unlock_all(win);
                    free(resbuf);
                } else {
                    /* TODO: add a DTP_buf_set() function to replace this */
                    char *tmp = (char *) calloc(lb + extent * targetcount, sizeof(char));
                    memcpy(tmp, targetbuf, lb + extent * targetcount);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                    }
                    /* restore target buffer */
                    memcpy(targetbuf, tmp, lb + extent * targetcount);
                    free(tmp);

                    MPI_Barrier(comm);
                    MPI_Win_unlock(rank, win);

                    /*signal the source that checking and reinitialization is done */
                    MPI_Barrier(comm);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                    }
                    MPI_Barrier(comm);
                    MPI_Win_unlock(rank, win);
                }
                DTP_obj_free(orig_dtp, j);
            }
            MPI_Win_free(&win);
            DTP_obj_free(target_dtp, i);
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
