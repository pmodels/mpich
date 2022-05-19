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
static char MTEST_Descrip[] = "Send-Recv";
*/

int world_rank, world_size;

static int sendrecv1(int seed, int testsize, int sendcnt, int recvcnt, const char *basic_type,
                     mtest_mem_type_e sendmem, mtest_mem_type_e recvmem, int source)
{
    int errs = 0;
    int err;
    int rank, size;
    MPI_Aint sendcount, recvcount;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    DTP_pool_s dtp;
    struct mtest_obj send, recv;

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./sendrecv1 -seed=%d -testsize=%d -type=%s -sendcnt=%d -recvcnt=%d -sendmem=%s -recvmem=%s",
             seed, testsize, basic_type, sendcnt, recvcnt, MTest_memtype_name(sendmem),
             MTest_memtype_name(recvmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating dtp pool (%s,%d)\n", basic_type, sendcnt);
        fflush(stderr);
    }

    MTest_dtp_obj_start(&send, "send", dtp, sendmem, 0, false);
    MTest_dtp_obj_start(&recv, "recv", dtp, recvmem, 0, false);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, 2, 1)) {
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

        int dest = (source + size - 1) % size;

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (int i = 0; i < testsize; i++) {
            DTP_pool_update_count(dtp, (rank == source) ? sendcnt : recvcnt);
            errs += MTest_dtp_create(&send, rank == source);
            errs += MTest_dtp_create(&recv, rank == dest);

            if (rank == source) {
                MTest_dtp_init(&send, 0, 1, sendcnt);

                sendcount = send.dtp_obj.DTP_type_count;
                sendtype = send.dtp_obj.DTP_datatype;

                err = MPI_Send((const char *) send.buf + send.dtp_obj.DTP_buf_offset,
                               sendcount, sendtype, dest, 0, comm);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }
            } else if (rank == dest) {
                MTest_dtp_init(&recv, -1, -1, recvcnt);

                recvcount = recv.dtp_obj.DTP_type_count;
                recvtype = recv.dtp_obj.DTP_datatype;

                MPI_Status status;
                err = MPI_Recv((char *) recv.buf + recv.dtp_obj.DTP_buf_offset,
                               recvcount, recvtype, source, 0, comm, &status);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }

                /* only up to sendcnt should be updated */
                errs += MTestCheckStatus(&status, dtp.DTP_base_type, sendcnt, source, 0, errs < 10);
                errs += MTest_dtp_check(&recv, 0, 1, sendcnt, &send, errs < 10);
            }
            MTest_dtp_destroy(&send);
            MTest_dtp_destroy(&recv);
        }
        MTestFreeComm(&comm);
    }

    MTest_dtp_obj_finish(&send);
    MTest_dtp_obj_finish(&recv);
    DTP_pool_free(dtp);
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    if (world_size < 2) {
        printf("This test requires 2 processes\n");
        errs++;
        goto fn_exit;
    }

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_PT2PT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        for (int source_rank = 0; source_rank < 2; source_rank++) {
            MTestPrintfMsg(1, "Test with source rank = %d\n", source_rank);
            errs += sendrecv1(dtp_args.seed, dtp_args.testsize,
                              dtp_args.count, dtp_args.u.pt2pt.recvcnt,
                              dtp_args.basic_type, dtp_args.u.pt2pt.sendmem,
                              dtp_args.u.pt2pt.recvmem, source_rank);
        }
    }
    dtp_args_finalize(&dtp_args);

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
