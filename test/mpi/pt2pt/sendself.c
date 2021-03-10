/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Test of sending to self (with a preposted receive)";
*/

static int sendself(int seed, int testsize, int sendcnt, int recvcnt,
                    const char *basic_type, mtest_mem_type_e t_sendmem, mtest_mem_type_e t_recvmem)
{
    int errs = 0, err;
    int rank, size;
    int i, j, len;
    MPI_Aint sendcount, recvcount;
    MPI_Aint maxbufsize;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    MPI_Request req;
    DTP_pool_s dtp;
    MTEST_DTP_DECLARE(send);
    MTEST_DTP_DECLARE(recv);

    sendmem = t_sendmem;
    recvmem = t_recvmem;

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

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%d)\n", basic_type, sendcnt);
        fflush(stderr);
    }

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    for (i = 0; i < testsize; i++) {
        err = DTP_obj_create(dtp, &send_obj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
        }

        err = DTP_obj_create(dtp, &recv_obj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
        }

        MTest_dtp_malloc_obj(send, 0);
        MTest_dtp_malloc_obj(recv, 1);

        MTest_dtp_init(send, 0, 1, sendcnt);
        MTest_dtp_init(recv, -1, -1, recvcnt);

        sendcount = send_obj.DTP_type_count;
        sendtype = send_obj.DTP_datatype;

        recvcount = recv_obj.DTP_type_count;
        recvtype = recv_obj.DTP_datatype;

        err =
            MPI_Irecv(recvbuf + recv_obj.DTP_buf_offset, recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Send(sendbuf + send_obj.DTP_buf_offset, sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, MPI_STATUS_IGNORE);

        MTest_dtp_check(recv, 0, 1, sendcnt);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                    recv_desc, send_desc, sendcnt);
            fflush(stderr);
            free(recv_desc);
            free(send_desc);
        }

        MTest_dtp_init(recv, -1, -1, sendcnt);

        err =
            MPI_Irecv(recvbuf + recv_obj.DTP_buf_offset, recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Ssend(sendbuf + send_obj.DTP_buf_offset, sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, MPI_STATUS_IGNORE);

        MTest_dtp_check(recv, 0, 1, sendcnt);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                    recv_desc, send_desc, sendcnt);
            fflush(stderr);
            free(recv_desc);
            free(send_desc);
        }

        MTest_dtp_init(recv, -1, -1, sendcnt);

        err =
            MPI_Irecv(recvbuf + recv_obj.DTP_buf_offset, recvcount, recvtype, rank, 0, comm, &req);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Rsend(sendbuf + send_obj.DTP_buf_offset, sendcount, sendtype, rank, 0, comm);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }

        err = MPI_Wait(&req, MPI_STATUS_IGNORE);

        MTest_dtp_check(recv, 0, 1, sendcnt);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                    recv_desc, send_desc, sendcnt);
            fflush(stderr);
            free(recv_desc);
            free(send_desc);
        }

        MTest_dtp_free(send);
        MTest_dtp_free(recv);
        DTP_obj_free(send_obj);
        DTP_obj_free(recv_obj);
    }

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
