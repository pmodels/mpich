/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mtest_dtp.h"
#include "dtpools.h"
#include <assert.h>

/* Extended from src/pt2pt/pingping.c with partitioned APIs. */

/* the stress testing of unexpected message queue is covered by the pt2pt/pingping,
 * test. Use barrier to simplify this test to focus on testing partitioned
 * communications.
 */
#define USE_BARRIER 1

/*
static char MTEST_Descrip[] = "Pingping test with partitioned point-to-point routines";
*/

#define MAX_TOTAL_MSG_SIZE (32 * 1024 * 1024)
#define MAXMSG (4096)

/* Used on both sender and receiver sides. If the total count of datatype elements
 * cannot be completely divided, we set local partitions to 1. */
#define PARTITIONS 4

/* Utility functions */
static void set_partitions_count(int total_count, int *partitions, MPI_Count * count)
{
    if (total_count % PARTITIONS == 0) {
        *partitions = PARTITIONS;
        *count = total_count / PARTITIONS;      /* count per partition */
    } else {
        *partitions = 1;
        *count = total_count;   /* count per partition */
    }
}

/* Test subroutines */
static void send_test(void *sbuf, int partitions, MPI_Count count, MPI_Datatype stype,
                      int dest, MPI_Comm comm)
{
    MPI_Request req = MPI_REQUEST_NULL;

    MPI_Psend_init(sbuf, partitions, count, stype, dest, 0, comm, MPI_INFO_NULL, &req);
    MPI_Start(&req);

    /* Set partition ready separately to test partial data transfer */
    for (int i = 0; i < partitions; i++)
        MPI_Pready(i, req);

    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);
}

static void recv_test(void *rbuf, int partitions, MPI_Count count, MPI_Datatype rtype,
                      int source, MPI_Comm comm, MPI_Status * p_status)
{
    MPI_Request req = MPI_REQUEST_NULL;

    MPI_Precv_init(rbuf, partitions, count, rtype, source, 0, comm, MPI_INFO_NULL, &req);
    MPI_Start(&req);

    MPI_Wait(&req, p_status);
    MPI_Request_free(&req);
}

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, nmsg, maxmsg;
    int seed, testsize;
    MPI_Aint sendcnt, recvcnt;
    MPI_Comm comm;
    DTP_pool_s dtp;
    struct mtest_obj send, recv;
    mtest_mem_type_e sendmem, recvmem;
    char *basic_type;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    sendcnt = MTestArgListGetLong(head, "sendcnt");
    recvcnt = MTestArgListGetLong(head, "recvcnt");
    basic_type = MTestArgListGetString(head, "type");
    sendmem = MTestArgListGetMemType(head, "sendmem");
    recvmem = MTestArgListGetMemType(head, "recvmem");

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%ld)\n", basic_type, sendcnt);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

    MTest_dtp_obj_start(&send, "send", dtp, sendmem, 0, false);
    MTest_dtp_obj_start(&recv, "recv", dtp, recvmem, 0, false);

    int nbytes;
    MPI_Type_size(dtp.DTP_base_type, &nbytes);
    nbytes *= sendcnt;
    maxmsg = MAX_TOTAL_MSG_SIZE / nbytes;
    if (maxmsg > MAXMSG)
        maxmsg = MAXMSG;

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
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
        source = 0;
        dest = size - 1;

        DTP_pool_update_count(dtp, rank == source ? sendcnt : recvcnt);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (int i = 0; i < testsize; i++) {
            errs += MTest_dtp_create(&send, rank == source);
            errs += MTest_dtp_create(&recv, rank == dest);

            int partitions = 0;
            MPI_Count partition_count = 0;
            MPI_Datatype dtype = MPI_DATATYPE_NULL;

            if (rank == source) {
                MTest_dtp_init(&send, 0, 1, sendcnt);

                dtype = send.dtp_obj.DTP_datatype;
                set_partitions_count(send.dtp_obj.DTP_type_count, &partitions, &partition_count);

                MTestPrintfMsg(1,
                               "Sending partitions = %d, count = %ld (total size %d bytes) of datatype %s",
                               partitions, partition_count, nbytes,
                               DTP_obj_get_description(send.dtp_obj));

                for (nmsg = 1; nmsg < maxmsg; nmsg++)
                    send_test((char *) send.buf + send.dtp_obj.DTP_buf_offset, partitions,
                              partition_count, dtype, dest, comm);
            } else if (rank == dest) {
                dtype = recv.dtp_obj.DTP_datatype;
                set_partitions_count(recv.dtp_obj.DTP_type_count, &partitions, &partition_count);

                MTestPrintfMsg(1, "Receiving partitions = %d, count = %d of datatype %s\n",
                               partitions, partition_count, DTP_obj_get_description(recv.dtp_obj));

                for (nmsg = 1; nmsg < maxmsg; nmsg++) {
                    MTest_dtp_init(&recv, -1, -1, recvcnt);

                    MPI_Status status;
                    recv_test((char *) recv.buf + recv.dtp_obj.DTP_buf_offset, partitions,
                              partition_count, dtype, source, comm, &status);

                    errs += MTestCheckStatus(&status, dtp.DTP_base_type, sendcnt, source, 0,
                                             errs < 10);
                    errs += MTest_dtp_check(&recv, 0, 1, sendcnt, &send, errs < 10);
                }
            }
            MTest_dtp_destroy(&send);
            MTest_dtp_destroy(&recv);
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

    MTest_dtp_obj_finish(&send);
    MTest_dtp_obj_finish(&recv);
    DTP_pool_free(dtp);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
