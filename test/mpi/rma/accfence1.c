/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Accumulate/Replace with Fence";
*/

#define error(...)                              \
    do {                                        \
        fprintf(stderr, __VA_ARGS__);           \
        fflush(stderr);                         \
    } while (0)

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, orig, target;
    int minsize = 2;
    int i;
    int seed, testsize;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint extent, lb, count, maxbufsize;
    MPI_Datatype origtype, targettype;
    DTP_pool_s dtp;
    DTP_obj_s orig_obj, target_obj;
    void *origbuf, *targetbuf;
    char *basic_type;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        error("Error while creating orig pool (%s,%ld)\n", basic_type, count);
    }

    MTestArgListDestroy(head);

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
    } else {
        /* accumulate tests cannot use struct types */
        goto fn_exit;
    }

    targetbuf = malloc(maxbufsize);
    if (targetbuf == NULL)
        errs++;

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            continue;
        }

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig = 0;
        target = size - 1;

        MPI_Win_create(targetbuf, maxbufsize, extent, MPI_INFO_NULL, comm, &win);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        for (i = 0; i < testsize; i++) {
            err = DTP_obj_create(dtp, &orig_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            err = DTP_obj_create(dtp, &target_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            err = DTP_obj_buf_init(target_obj, targetbuf, -1, -1, count);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            targetcount = target_obj.DTP_type_count;
            targettype = target_obj.DTP_datatype;

            MPI_Win_fence(0, win);

            if (rank == orig) {
                origbuf = malloc(orig_obj.DTP_bufsize);
                if (origbuf == NULL) {
                    errs++;
                    break;
                }

                err = DTP_obj_buf_init(orig_obj, origbuf, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                origcount = orig_obj.DTP_type_count;
                origtype = orig_obj.DTP_datatype;

                /* MPI_REPLACE on accumulate is almost the same
                 * as MPI_Put; the only difference is in the
                 * handling of overlapping accumulate operations,
                 * which are not tested here */
                err = MPI_Accumulate(origbuf + orig_obj.DTP_buf_offset, origcount,
                                     origtype, target, target_obj.DTP_buf_offset / extent,
                                     targetcount, targettype, MPI_REPLACE, win);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        char *orig_desc, *target_desc;
                        DTP_obj_get_description(orig_obj, &orig_desc);
                        DTP_obj_get_description(target_obj, &target_desc);
                        error("Accumulate types: send %s, recv %s\n", orig_desc, target_desc);
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

                free(origbuf);
            } else if (rank == target) {
                MPI_Win_fence(0, win);
                /* This should have the same effect, in terms of
                 * transfering data, as a send/recv pair */
                err = DTP_obj_buf_check(target_obj, targetbuf, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    char *orig_desc, *target_desc;
                    DTP_obj_get_description(orig_obj, &orig_desc);
                    DTP_obj_get_description(target_obj, &target_desc);
                    if (errs < 10) {
                        error("Data received with type %s does not match data sent with type %s\n",
                              target_desc, orig_desc);
                    }
                    errs++;
                }
            } else {
                MPI_Win_fence(0, win);
            }
            DTP_obj_free(orig_obj);
            DTP_obj_free(target_obj);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    free(targetbuf);

  fn_exit:
    DTP_pool_free(dtp);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
