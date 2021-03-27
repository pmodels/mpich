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
static char MTEST_Descrip[] = "Test of sending to self (with a preposted receive)";
*/



int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size;
    int i, j, len, seed, testsize;
    MPI_Aint sendcount, recvcount, count[2];
    MPI_Aint maxbufsize;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    MPI_Request req;
    DTP_pool_s dtp;
    MTEST_DTP_DECLARE(send);
    MTEST_DTP_DECLARE(recv);
    char *basic_type;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count[0] = MTestArgListGetLong(head, "sendcnt");
    count[1] = MTestArgListGetLong(head, "recvcnt");
    basic_type = MTestArgListGetString(head, "type");
    sendmem = MTestArgListGetMemType(head, "sendmem");
    recvmem = MTestArgListGetMemType(head, "recvmem");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count[0], seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%ld)\n", basic_type, count[0]);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

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

        MTest_dtp_init(send, 0, 1, count[0]);
        MTest_dtp_init(recv, -1, -1, count[0]);

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

        MTest_dtp_check(recv, 0, 1, count[0]);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %ld\n",
                    recv_desc, send_desc, count[0]);
            fflush(stderr);
            free(recv_desc);
            free(send_desc);
        }

        MTest_dtp_init(recv, -1, -1, count[0]);

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

        MTest_dtp_check(recv, 0, 1, count[0]);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %ld\n",
                    recv_desc, send_desc, count[0]);
            fflush(stderr);
            free(recv_desc);
            free(send_desc);
        }

        MTest_dtp_init(recv, -1, -1, count[0]);

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

        MTest_dtp_check(recv, 0, 1, count[0]);
        if (err != DTP_SUCCESS && errs <= 10) {
            char *recv_desc, *send_desc;
            DTP_obj_get_description(recv_obj, &recv_desc);
            DTP_obj_get_description(send_obj, &send_desc);
            fprintf(stderr,
                    "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %ld\n",
                    recv_desc, send_desc, count[0]);
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

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
