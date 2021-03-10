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
static char MTEST_Descrip[] = "Send flood test";
*/

int world_rank, world_size;

#define MAX_TOTAL_MSG_SIZE (32 * 1024 * 1024)
#define MAXMSG (4096)

static int pingping(int seed, int testsize, int sendcnt, int recvcnt,
                    const char *basic_type, mtest_mem_type_e t_sendmem, mtest_mem_type_e t_recvmem)
{
    int errs = 0;
    int err;
    int rank, size, source, dest;
    int minsize = 2, nmsg, maxmsg;
    int i, j, len;
    MPI_Aint sendcount, recvcount;
    MPI_Aint maxbufsize;
    MPI_Comm comm;
    MPI_Datatype sendtype, recvtype;
    DTP_pool_s dtp;
    MTEST_DTP_DECLARE(send);
    MTEST_DTP_DECLARE(recv);

    sendmem = t_sendmem;
    recvmem = t_recvmem;

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./pingping -seed=%d -testsize=%d -type=%s -sendcnt=%d -recvcnt=%d -sendmem=%s -recvmem=%s",
             seed, testsize, basic_type, sendcnt, recvcnt, MTest_memtype_name(sendmem),
             MTest_memtype_name(recvmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%d)\n", basic_type, sendcnt);
        fflush(stderr);
    }

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
        source = 0;
        dest = size - 1;

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (i = 0; i < testsize; i++) {
            err = DTP_obj_create(dtp, &send_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            err += DTP_obj_create(dtp, &recv_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            int nbytes;
            MPI_Type_size(send_obj.DTP_datatype, &nbytes);
            nbytes *= send_obj.DTP_type_count;

            maxmsg = MAX_TOTAL_MSG_SIZE / nbytes;
            if (maxmsg > MAXMSG)
                maxmsg = MAXMSG;

            if (rank == source) {
                MTest_dtp_malloc_obj(send, rank);
                MTest_dtp_init(send, 0, 1, sendcnt);

                sendcount = send_obj.DTP_type_count;
                sendtype = send_obj.DTP_datatype;

                char *desc;
                DTP_obj_get_description(send_obj, &desc);
                MTestPrintfMsg(1, "Sending count = %d of sendtype %s of total size %d bytes\n",
                               sendcnt, desc, nbytes * sendcnt);
                free(desc);

                for (nmsg = 1; nmsg < maxmsg; nmsg++) {
                    err =
                        MPI_Send(sendbuf + send_obj.DTP_buf_offset, sendcount, sendtype, dest, 0,
                                 comm);
                    if (err) {
                        errs++;
                        if (errs < 10) {
                            MTestPrintError(err);
                        }
                    }
                }

                MTest_dtp_free(send);
            } else if (rank == dest) {
                MTest_dtp_malloc_obj(recv, rank);

                recvcount = recv_obj.DTP_type_count;
                recvtype = recv_obj.DTP_datatype;

                for (nmsg = 1; nmsg < maxmsg; nmsg++) {
                    MTest_dtp_init(recv, -1, -1, recvcnt);

                    err =
                        MPI_Recv(recvbuf + recv_obj.DTP_buf_offset, recvcount, recvtype, source, 0,
                                 comm, MPI_STATUS_IGNORE);
                    if (err) {
                        errs++;
                        if (errs < 10) {
                            MTestPrintError(err);
                        }
                    }

                    MTest_dtp_check(recv, 0, 1, recvcnt);
                    if (err != DTP_SUCCESS && errs < 10) {
                        char *recv_desc, *send_desc;
                        DTP_obj_get_description(recv_obj, &recv_desc);
                        DTP_obj_get_description(send_obj, &send_desc);
                        fprintf(stderr,
                                "Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d, message iteration %d of %d\n",
                                recv_desc, send_desc, recvcnt, nmsg, maxmsg);
                        fflush(stderr);
                        free(recv_desc);
                        free(send_desc);
                    }
                }

                MTest_dtp_free(recv);
            }
            DTP_obj_free(recv_obj);
            DTP_obj_free(send_obj);
#ifdef USE_BARRIER
            /* NOTE: Without MPI_Barrier, recv side can easily accumulate large unexpected queue
             * across multiple batches, especially in an async test. Currently, both libfabric and ucx
             * netmod does not handle large message queue well, resulting in exponential slow-downs.
             * Adding barrier let the current tests pass.
             */
            /* FIXME: fix netmod issues then remove the barrier (and corresponding tests). */
            MPI_Barrier(comm);
#endif
        }
        MTestFreeComm(&comm);
    }

    DTP_pool_free(dtp);
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_PT2PT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += pingping(dtp_args.seed, dtp_args.testsize,
                         dtp_args.count, dtp_args.u.pt2pt.recvcnt,
                         dtp_args.basic_type, dtp_args.u.pt2pt.sendmem, dtp_args.u.pt2pt.recvmem);
    }
    dtp_args_finalize(&dtp_args);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
