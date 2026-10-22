/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run part_pingping
int run(const char *arg);
#endif

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
#define MAXMSG (16)
#define MAXMSG_STRESS (4096)

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

static int pingping(int seed, int testsize, int sendcnt, int recvcnt,
                    const char *basic_type, mtest_mem_type_e sendmem, mtest_mem_type_e recvmem)
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, nmsg, maxmsg;
    MPI_Comm comm;
    DTP_pool_s dtp;
    struct mtest_obj send, recv;

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./pingping -seed=%d -testsize=%d -type=%s -sendcnt=%d -recvcnt=%d -sendmem=%s -recvmem=%s",
             seed, testsize, basic_type, sendcnt, recvcnt, MTest_memtype_name(sendmem),
             MTest_memtype_name(recvmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, sendcnt, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating send pool (%s,%d)\n", basic_type, sendcnt);
        fflush(stderr);
    }

    MTest_dtp_obj_start(&send, "send", dtp, sendmem, 0, false);
    MTest_dtp_obj_start(&recv, "recv", dtp, recvmem, 0, false);

    int nbytes;
    MPI_Type_size(dtp.DTP_base_type, &nbytes);
    nbytes *= sendcnt;
    maxmsg = MAX_TOTAL_MSG_SIZE / nbytes;
    int max_nmsg = MTestGetStressLevel()? MAXMSG_STRESS : MAXMSG;
    if (maxmsg > max_nmsg)
        maxmsg = max_nmsg;

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
            if (MTestGetStressLevel() == 0) {
                /* NOTE: Without MPI_Barrier, recv side can easily accumulate large unexpected queue
                 * across multiple batches, especially in an async test. Currently, both libfabric and ucx
                 * netmod does not handle large message queue well, resulting in exponential slow-downs.
                 * Adding barrier let the current tests pass.
                 */
                MPI_Barrier(comm);
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_dtp_obj_finish(&send);
    MTest_dtp_obj_finish(&recv);
    DTP_pool_free(dtp);
    return errs;
}

int run(const char *arg)
{
    int errs = 0;

    struct dtp_args dtp_args;
    dtp_args_init_arg(&dtp_args, MTEST_DTP_PT2PT, arg);
    while (dtp_args_get_next(&dtp_args)) {
        errs += pingping(dtp_args.seed, dtp_args.testsize,
                         dtp_args.count, dtp_args.u.pt2pt.recvcnt,
                         dtp_args.basic_type, dtp_args.u.pt2pt.sendmem, dtp_args.u.pt2pt.recvmem);
    }
    dtp_args_finalize(&dtp_args);

    return errs;
}
