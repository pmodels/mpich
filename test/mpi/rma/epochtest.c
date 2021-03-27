/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This test looks at the behavior of MPI_Win_fence and epochs.  Each
 * MPI_Win_fence may both begin and end both the exposure and access epochs.
 * Thus, it is not necessary to use MPI_Win_fence in pairs.
 *
 * The tests have this form:
 *    Process A             Process B
 *     fence                 fence
 *      put,put
 *     fence                 fence
 *                            put,put
 *     fence                 fence
 *      put,put               put,put
 *     fence                 fence
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Put with Fences used to separate epochs";
*/

#define MAX_PERR 10


int PrintRecvedError(const char *, const char *, const char *);

int main(int argc, char **argv)
{
    int errs = 0, err;
    int rank, size, orig, target;
    int minsize = 2;
    int i;
    int seed, testsize;
    int onlyInt = 0;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint extent, lb, count, maxbufsize;
    MPI_Datatype origtype, targettype;
    DTP_pool_s dtp;
    MTEST_DTP_DECLARE(orig);
    MTEST_DTP_DECLARE(target);
    char *basic_type;

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

    /* Check for a simple choice of communicator and datatypes */
    if (getenv("MTEST_SIMPLE"))
        onlyInt = 1;

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
    } else {
        /* if the base datatype is not a basic datatype, use an extent
         * of 1 */
        extent = 1;
    }

    MTest_dtp_malloc_max(target, 0);

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

            MTest_dtp_malloc_obj(orig, 1);
            MTest_dtp_init(orig, 0, 1, count);
            MTest_dtp_init(target, -1, -1, count);

            origcount = orig_obj.DTP_type_count;
            origtype = orig_obj.DTP_datatype;

            targetcount = target_obj.DTP_type_count;
            targettype = target_obj.DTP_datatype;

            char *orig_desc, *target_desc;
            DTP_obj_get_description(orig_obj, &orig_desc);
            DTP_obj_get_description(target_obj, &target_desc);
            MTestPrintfMsg(1,
                           "Putting count = %d of origtype %s targettype %s\n",
                           count, orig_desc, target_desc);

            /* At this point, we have all of the elements that we
             * need to begin the multiple fence and put tests */
            /* Fence 1 */
            err = MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
            if (err) {
                if (errs++ < MAX_PERR)
                    MTestPrintError(err);
            }

            if (rank == orig) {
                err =
                    MPI_Put(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, target,
                            target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
                if (err) {
                    if (errs++ < MAX_PERR)
                        MTestPrintError(err);
                }
            }

            /* Fence 2 */
            err = MPI_Win_fence(0, win);
            if (err) {
                if (errs++ < MAX_PERR)
                    MTestPrintError(err);
            }
            /* target checks data, then target puts */
            if (rank == target) {
                MTest_dtp_check(target, 0, 1, count);
                if (err && errs < MAX_PERR) {
                    PrintRecvedError("fence2", orig_desc, target_desc);
                }

                err =
                    MPI_Put(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, orig,
                            target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
                if (err) {
                    if (errs++ < MAX_PERR)
                        MTestPrintError(err);
                }
            }

            /* Fence 3 */
            err = MPI_Win_fence(0, win);
            if (err) {
                if (errs++ < MAX_PERR)
                    MTestPrintError(err);
            }
            /* src checks data, then Src and target puts */
            if (rank == orig) {
                MTest_dtp_check(target, 0, 1, count);
                if (err != DTP_SUCCESS && errs < MAX_PERR) {
                    PrintRecvedError("fence3", orig_desc, target_desc);
                }

                err =
                    MPI_Put(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, target,
                            target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
                if (err) {
                    if (errs++ < MAX_PERR)
                        MTestPrintError(err);
                }
            }
            if (rank == target) {
                err =
                    MPI_Put(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, orig,
                            target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
                if (err) {
                    if (errs++ < MAX_PERR)
                        MTestPrintError(err);
                }
            }

            /* Fence 4 */
            err = MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
            if (err) {
                if (errs++ < MAX_PERR)
                    MTestPrintError(err);
            }
            /* src and target checks data */
            if (rank == orig) {
                MTest_dtp_check(target, 0, 1, count);
                if (err != DTP_SUCCESS && errs < MAX_PERR) {
                    PrintRecvedError("src fence4", orig_desc, target_desc);
                }
            }
            if (rank == target) {
                MTest_dtp_check(target, 0, 1, count);
                if (err != DTP_SUCCESS && errs < MAX_PERR) {
                    PrintRecvedError("target fence4", orig_desc, target_desc);
                }
            }

            free(orig_desc);
            free(target_desc);
            MTest_dtp_free(orig);
            DTP_obj_free(orig_obj);
            DTP_obj_free(target_obj);

            /* Only do one count in the simple case */
            if (onlyInt)
                break;
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
        /* Only do one communicator in the simple case */
        if (onlyInt)
            break;
    }

    MTest_dtp_free(target);
    DTP_pool_free(dtp);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

int PrintRecvedError(const char *msg, const char *orig_name, const char *target_name)
{
    printf
        ("At step %s, Data in target buffer did not match for destination datatype %s (put with orig datatype %s)\n",
         msg, orig_name, target_name);
    return 0;
}
