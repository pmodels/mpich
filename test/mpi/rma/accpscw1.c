/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Accumulate/replace with Post/Start/Complete/Wait";
*/



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
    MPI_Group wingroup, neighbors;
    MPI_Datatype origtype, targettype;
    DTP_pool_s dtp;
    DTP_obj_s orig_obj, target_obj;
    void *origbuf, *targetbuf;
    void *origbuf_h, *targetbuf_h;
    char *basic_type;
    mtest_mem_type_e origmem;
    mtest_mem_type_e targetmem;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");
    origmem = MTestArgListGetMemType(head, "origmem");
    targetmem = MTestArgListGetMemType(head, "targetmem");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%ld)\n", basic_type, count);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
    } else {
        /* accumulate tests cannot use struct types */
        goto fn_exit;
    }

    MTestAlloc(maxbufsize, targetmem, &targetbuf_h, &targetbuf, 0);
    assert(targetbuf && targetbuf_h);

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
        MPI_Win_get_group(win, &wingroup);

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

            err = DTP_obj_buf_init(target_obj, targetbuf_h, -1, -1, count);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }
            MTestCopyContent(targetbuf_h, targetbuf, maxbufsize, targetmem);

            targetcount = target_obj.DTP_type_count;
            targettype = target_obj.DTP_datatype;

            if (rank == orig) {
                MTestAlloc(orig_obj.DTP_bufsize, origmem, &origbuf_h, &origbuf, 0);
                assert(origbuf && origbuf_h);

                err = DTP_obj_buf_init(orig_obj, origbuf_h, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }
                MTestCopyContent(origbuf_h, origbuf, orig_obj.DTP_bufsize, origmem);

                origcount = orig_obj.DTP_type_count;
                origtype = orig_obj.DTP_datatype;

                /* Neighbor is target only */
                MPI_Group_incl(wingroup, 1, &target, &neighbors);
                err = MPI_Win_start(neighbors, 0, win);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }
                MPI_Group_free(&neighbors);
                err = MPI_Accumulate(origbuf + orig_obj.DTP_buf_offset, origcount,
                                     origtype, target, target_obj.DTP_buf_offset / extent,
                                     targetcount, targettype, MPI_REPLACE, win);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
                err = MPI_Win_complete(win);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }

                MTestFree(origmem, origbuf_h, origbuf);
            } else if (rank == target) {
                MPI_Group_incl(wingroup, 1, &orig, &neighbors);
                MPI_Win_post(neighbors, 0, win);
                MPI_Group_free(&neighbors);
                MPI_Win_wait(win);
                /* This should have the same effect, in terms of
                 * transfering data, as a send/recv pair */
                MTestCopyContent(targetbuf, targetbuf_h, maxbufsize, targetmem);
                err = DTP_obj_buf_check(target_obj, targetbuf_h, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    if (errs < 10) {
                        char *target_desc, *orig_desc;
                        DTP_obj_get_description(target_obj, &target_desc);
                        DTP_obj_get_description(orig_obj, &orig_desc);
                        fprintf(stderr,
                                "Data received with type %s does not match data sent with type %s\n",
                                target_desc, orig_desc);
                        fflush(stderr);
                        free(target_desc);
                        free(orig_desc);
                    }
                }
            } else {
                /* Nothing; the other processes need not call any
                 * MPI routines */
                ;
            }
            DTP_obj_free(orig_obj);
            DTP_obj_free(target_obj);
        }
        MPI_Win_free(&win);
        MPI_Group_free(&wingroup);
        MTestFreeComm(&comm);
    }

    MTestFree(targetmem, targetbuf_h, targetbuf);

  fn_exit:
    DTP_pool_free(dtp);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
