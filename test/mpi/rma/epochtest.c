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

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Put with Fences used to separate epochs";
*/

#define MAX_PERR 10

int world_rank, world_size;

int PrintRecvedError(const char *, const char *, const char *);

static int epoch_test(int seed, int testsize, int count, const char *basic_type,
                      mtest_mem_type_e origmem, mtest_mem_type_e targetmem)
{
    int errs = 0, err;
    int rank, size, orig_rank, target_rank;
    int minsize = 2;
    int i;
    int onlyInt = 0;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;
    DTP_pool_s dtp;
    struct mtest_obj orig, target;

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./epoch_test -seed=%d -testsize=%d -type=%s -count=%d -origmem=%s -targetmem=%s",
             seed, testsize, basic_type, count, MTest_memtype_name(origmem),
             MTest_memtype_name(targetmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%d)\n", basic_type, count);
        fflush(stderr);
    }

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

    MTest_dtp_obj_start(&orig, "origin", dtp, origmem, 0, false);
    MTest_dtp_obj_start(&target, "target", dtp, targetmem, 1, true);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            continue;
        }

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig_rank = 0;
        target_rank = size - 1;

        MPI_Win_create(target.buf, target.maxbufsize, extent, MPI_INFO_NULL, comm, &win);
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        for (i = 0; i < testsize; i++) {
            errs += MTest_dtp_create(&orig, true);
            errs += MTest_dtp_create(&target, false);

            errs += MTest_dtp_init(&orig, 0, 1, count);
            errs += MTest_dtp_init(&target, -1, -1, count);

            origcount = orig.dtp_obj.DTP_type_count;
            origtype = orig.dtp_obj.DTP_datatype;

            targetcount = target.dtp_obj.DTP_type_count;
            targettype = target.dtp_obj.DTP_datatype;

            /* At this point, we have all of the elements that we
             * need to begin the multiple fence and put tests */
            /* Fence 1 */
            err = MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
            if (err) {
                if (errs++ < MAX_PERR)
                    MTestPrintError(err);
            }

            if (rank == orig_rank) {
                err = MPI_Put((char *) orig.buf + orig.dtp_obj.DTP_buf_offset, origcount, origtype,
                              target_rank, target.dtp_obj.DTP_buf_offset / extent, targetcount,
                              targettype, win);
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
            if (rank == target_rank) {
                errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < MAX_PERR);

                err = MPI_Put((char *) orig.buf + orig.dtp_obj.DTP_buf_offset,
                              origcount, origtype, orig_rank,
                              target.dtp_obj.DTP_buf_offset / extent, targetcount, targettype, win);
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
            if (rank == orig_rank) {
                errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < MAX_PERR);

                err = MPI_Put((char *) orig.buf + orig.dtp_obj.DTP_buf_offset, origcount, origtype,
                              target_rank, target.dtp_obj.DTP_buf_offset / extent, targetcount,
                              targettype, win);
                if (err) {
                    if (errs++ < MAX_PERR)
                        MTestPrintError(err);
                }
            }
            if (rank == target_rank) {
                err = MPI_Put((char *) orig.buf + orig.dtp_obj.DTP_buf_offset,
                              origcount, origtype, orig_rank,
                              target.dtp_obj.DTP_buf_offset / extent, targetcount, targettype, win);
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
            if (rank == orig_rank) {
                errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < MAX_PERR);
            }
            if (rank == target_rank) {
                errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < MAX_PERR);
            }

            MTest_dtp_destroy(&orig);
            MTest_dtp_destroy(&target);

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

    MTest_dtp_obj_finish(&orig);
    MTest_dtp_obj_finish(&target);
    DTP_pool_free(dtp);

    return errs;
}

int PrintRecvedError(const char *msg, const char *orig_name, const char *target_name)
{
    printf
        ("At step %s, Data in target buffer did not match for destination datatype %s (put with orig datatype %s)\n",
         msg, orig_name, target_name);
    return 0;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_RMA, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += epoch_test(dtp_args.seed, dtp_args.testsize,
                           dtp_args.count, dtp_args.basic_type,
                           dtp_args.u.rma.origmem, dtp_args.u.rma.targetmem);

    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
