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

/* Extended from src/pt2pt/pingping.c with partitioned APIs. */

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
        MPI_Pready(i, &req);

    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);
}

static void recv_test(void *rbuf, int partitions, MPI_Count count, MPI_Datatype rtype,
                      int source, MPI_Comm comm)
{
    MPI_Request req = MPI_REQUEST_NULL;

    MPI_Precv_init(rbuf, partitions, count, rtype, source, 0, comm, MPI_INFO_NULL, &req);
    MPI_Start(&req);

    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);
}

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, nmsg, maxmsg;
    int seed, testsize;
    MPI_Aint count[2];
    MPI_Aint maxbufsize;
    MPI_Comm comm;
    DTP_pool_s dtp;
    DTP_obj_s send_obj, recv_obj;
    void *sendbuf, *recvbuf;
    void *sendbuf_h, *recvbuf_h;
    char *basic_type;
    mtest_mem_type_e sendmem;
    mtest_mem_type_e recvmem;

    MTest_Init(&argc, &argv);

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

        for (int i = 0; i < testsize; i++) {
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

            int partitions = 0;
            MPI_Count partition_count = 0;
            MPI_Datatype dtype = MPI_DATATYPE_NULL;

            if (rank == source) {
                MTestMalloc(send_obj.DTP_bufsize, sendmem, &sendbuf_h, &sendbuf, rank);
                assert(sendbuf && sendbuf_h);

                err = DTP_obj_buf_init(send_obj, sendbuf_h, 0, 1, count[0]);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                MTestCopyContent(sendbuf_h, sendbuf, send_obj.DTP_bufsize, sendmem);

                dtype = send_obj.DTP_datatype;
                set_partitions_count(send_obj.DTP_type_count, &partitions, &partition_count);

                char *desc;
                DTP_obj_get_description(send_obj, &desc);
                MTestPrintfMsg(1,
                               "Sending partitions = %d, count = %ld (total size %d bytes) of datatype %s",
                               partitions, partition_count, nbytes * count[0], desc);
                free(desc);

                for (nmsg = 1; nmsg < maxmsg; nmsg++)
                    send_test((char *) sendbuf + send_obj.DTP_buf_offset, partitions,
                              partition_count, dtype, dest, comm);

                MTestFree(sendmem, sendbuf_h, sendbuf);
            } else if (rank == dest) {
                MTestMalloc(recv_obj.DTP_bufsize, recvmem, &recvbuf_h, &recvbuf, rank);
                assert(recvbuf && recvbuf_h);

                dtype = recv_obj.DTP_datatype;
                set_partitions_count(recv_obj.DTP_type_count, &partitions, &partition_count);

                char *desc;
                DTP_obj_get_description(recv_obj, &desc);
                MTestPrintfMsg(1, "Receiving partitions = %d, count = %d of datatype %s\n",
                               partitions, partition_count, desc);
                free(desc);

                for (nmsg = 1; nmsg < maxmsg; nmsg++) {
                    err = DTP_obj_buf_init(recv_obj, recvbuf_h, -1, -1, count[0]);
                    if (err != DTP_SUCCESS) {
                        errs++;
                        break;
                    }
                    MTestCopyContent(recvbuf_h, recvbuf, recv_obj.DTP_bufsize, recvmem);

                    recv_test((char *) recvbuf + recv_obj.DTP_buf_offset, partitions,
                              partition_count, dtype, source, comm);

                    MTestCopyContent(recvbuf, recvbuf_h, recv_obj.DTP_bufsize, recvmem);
                    err = DTP_obj_buf_check(recv_obj, recvbuf_h, 0, 1, count[0]);
                    if (err != DTP_SUCCESS) {
                        if (errs < 10) {
                            char *recv_desc, *send_desc;
                            DTP_obj_get_description(recv_obj, &recv_desc);
                            DTP_obj_get_description(send_obj, &send_desc);
                            fprintf(stderr,
                                    "Data in receive buffer did not match for destination datatype %s and source datatype %s,"
                                    "partitions = %d, count = %ld, total count = %ld, message iteration %d of %d\n",
                                    recv_desc, send_desc, partitions, partition_count, count[0],
                                    nmsg, maxmsg);
                            fflush(stderr);
                            free(recv_desc);
                            free(send_desc);
                        }
                        errs++;
                    }
                }

                MTestFree(recvmem, recvbuf_h, recvbuf);
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

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
