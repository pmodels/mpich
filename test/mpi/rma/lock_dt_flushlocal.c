/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock+flush_local";
*/


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

    err = MTestInitBasicSignature(argc, argv, &count, &basic_type);
    if (err)
        return MTestReturnValue(1);

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

    err = MTestInitStructSignature(argc, argv, &basic_type_num, &basic_type_counts, &basic_types);
    if (err)
        return MTestReturnValue(1);

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
        /* Determine the sender and receiver */
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
                    MPI_Aint slb, sextent;
                    MPI_Type_get_extent(origtype, &slb, &sextent);

                    /* TODO: add a DTP_buf_set() function to replace this */
                    char *tmp = (char *) calloc(slb + sextent * origcount, sizeof(char));
                    memcpy(tmp, origbuf, slb + sextent * origcount);

                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    MPI_Accumulate(origbuf, origcount,
                                   origtype, target, 0, targetcount, targettype, MPI_REPLACE, win);
                    MPI_Win_flush_local(target, win);
                    /* reset the send buffer to test local completion */
                    memset(origbuf, 0, slb + sextent * origcount);
                    MPI_Win_unlock(target, win);
                    MPI_Barrier(comm);

                    /* restore orig buffer */
                    memcpy(origbuf, tmp, slb + sextent * origcount);
                    free(tmp);

                    char *resbuf = (char *) calloc(lb + extent * targetcount, sizeof(char));

                    /*wait for the destinations to finish checking and reinitializing the buffers */
                    MPI_Barrier(comm);

                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    MPI_Get_accumulate(origbuf, origcount,
                                       origtype, resbuf, targetcount, targettype,
                                       target, 0, targetcount, targettype, MPI_REPLACE, win);
                    MPI_Win_flush_local(target, win);
                    /* reset the send buffer to test local completion */
                    memset(origbuf, 0, slb + sextent * origcount);
                    MPI_Win_unlock(target, win);
                    MPI_Barrier(comm);
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

                    MPI_Win_unlock(target, win);

                    /*signal the source that checking and reinitialization is done */
                    MPI_Barrier(comm);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        errs++;
                    }
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
