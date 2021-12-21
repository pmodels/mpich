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
static char MTEST_Descrip[] = "Test of sending to self (with a preposted receive)";
*/

static int sendself(int seed, int testsize, int sendcnt, int recvcnt,
                    const char *basic_type, mtest_mem_type_e sendmem, mtest_mem_type_e recvmem)
{
    int errs = 0, err;
    int rank, size;
    MPI_Aint sendcount, recvcount;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    MPI_Request req;
    DTP_pool_s dtp;
    struct mtest_obj send, recv;

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./sendrecv1 -seed=%d -testsize=%d -type=%s -sendcnt=%d -recvcnt=%d -sendmem=%s -recvmem=%s",
             seed, testsize, basic_type, sendcnt, recvcnt, MTest_memtype_name(sendmem),
             MTest_memtype_name(recvmem));
    if (rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%d)\n", basic_type, sendcnt);
        fflush(stderr);
    }

    MTest_dtp_obj_start(&send, "send", dtp, sendmem, 0, false);
    MTest_dtp_obj_start(&recv, "recv", dtp, recvmem, 0, false);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    for (int i = 0; i < testsize; i++) {
        MPI_Status status;

        DTP_pool_update_count(dtp, sendcnt);
        errs += MTest_dtp_create(&send, true);
        DTP_pool_update_count(dtp, recvcnt);
        errs += MTest_dtp_create(&recv, true);

        MTest_dtp_init(&send, 0, 1, sendcnt);
        MTest_dtp_init(&recv, -1, -1, recvcnt);

        sendcount = send.dtp_obj.DTP_type_count;
        sendtype = send.dtp_obj.DTP_datatype;

        recvcount = recv.dtp_obj.DTP_type_count;
        recvtype = recv.dtp_obj.DTP_datatype;

        err = MPI_Irecv((char *) recv.buf + recv.dtp_obj.DTP_buf_offset,
                        recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Send((const char *) send.buf + send.dtp_obj.DTP_buf_offset,
                       sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, &status);

        errs += MTestCheckStatus(&status, dtp.DTP_base_type, sendcnt, rank, 0, errs < 10);
        errs += MTest_dtp_check(&recv, 0, 1, sendcnt, &send, errs < 10);

        MTest_dtp_init(&recv, -1, -1, sendcnt);

        err = MPI_Irecv((char *) recv.buf + recv.dtp_obj.DTP_buf_offset,
                        recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Ssend((char *) send.buf + send.dtp_obj.DTP_buf_offset,
                        sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, &status);

        errs += MTestCheckStatus(&status, dtp.DTP_base_type, sendcnt, rank, 0, errs < 10);
        errs += MTest_dtp_check(&recv, 0, 1, sendcnt, &send, errs < 10);

        MTest_dtp_init(&recv, -1, -1, sendcnt);

        err = MPI_Irecv((char *) recv.buf + recv.dtp_obj.DTP_buf_offset,
                        recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Rsend((const char *) send.buf + send.dtp_obj.DTP_buf_offset,
                        sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, &status);

        errs += MTestCheckStatus(&status, dtp.DTP_base_type, sendcnt, rank, 0, errs < 10);
        errs += MTest_dtp_check(&recv, 0, 1, sendcnt, &send, errs < 10);

        MTest_dtp_destroy(&send);
        MTest_dtp_destroy(&recv);
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

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_PT2PT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += sendself(dtp_args.seed, dtp_args.testsize,
                         dtp_args.count, dtp_args.u.pt2pt.recvcnt,
                         dtp_args.basic_type, dtp_args.u.pt2pt.sendmem, dtp_args.u.pt2pt.recvmem);
    }
    dtp_args_finalize(&dtp_args);
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
