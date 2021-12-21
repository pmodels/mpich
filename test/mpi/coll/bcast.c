/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Test of broadcast with various roots and datatypes";
*/

static int bcast_dtp(int seed, int testsize, int count, const char *basic_type,
                     mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int errs = 0, err;
    int rank, size, root;
    int minsize = 2;
    MPI_Comm comm;
    DTP_pool_s dtp;
    struct mtest_obj coll;
    mtest_mem_type_e memtype;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./bcast -seed=%d -testsize=%d -type=%s -count=%d -evenmemtype=%s -oddmemtype=%s",
             seed, testsize, basic_type, count, MTest_memtype_name(evenmem),
             MTest_memtype_name(oddmem));
    if (rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    if (rank % 2 == 0)
        memtype = evenmem;
    else
        memtype = oddmem;

    err = DTP_pool_create(basic_type, count, seed + rank, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%d)\n", basic_type, count);
        fflush(stderr);
    }

    MTest_dtp_obj_start(&coll, "coll", dtp, memtype, rank, false);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
#if defined BCAST_COMM_WORLD_ONLY
        if (comm != MPI_COMM_WORLD) {
            MTestFreeComm(&comm);
            continue;
        }
#endif /* BCAST_COMM_WORLD_ONLY */

        if (comm == MPI_COMM_NULL)
            continue;

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (root = 0; root < size; root++) {
            for (int i = 0; i < testsize; i++) {
                errs += MTest_dtp_create(&coll, true);

                if (rank == root) {
                    errs += MTest_dtp_init(&coll, 0, 1, count);
                } else {
                    errs += MTest_dtp_init(&coll, -1, -1, count);
                }

                err = MPI_Bcast((char *) coll.buf + coll.dtp_obj.DTP_buf_offset,
                                coll.dtp_obj.DTP_type_count, coll.dtp_obj.DTP_datatype, root, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }

                errs += MTest_dtp_check(&coll, 0, 1, count, NULL, errs < 10);

                MTest_dtp_destroy(&coll);
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_dtp_obj_finish(&coll);
    DTP_pool_free(dtp);
    return errs;
}


int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_COLL, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += bcast_dtp(dtp_args.seed, dtp_args.testsize,
                          dtp_args.count, dtp_args.basic_type,
                          dtp_args.u.coll.oddmem, dtp_args.u.coll.evenmem);
    }
    dtp_args_finalize(&dtp_args);
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
