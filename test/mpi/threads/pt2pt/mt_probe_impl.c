/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "mt_pt2pt_common.h"

enum operation_types { TYPE_ASYNC, TYPE_SYNC };
enum probe_routines { Probe, Iprobe, Mprobe, Improbe };

/* Setup send and recv types */
#ifdef BLOCKINGSEND
#define SEND_TYPE TYPE_SYNC
#else
#define SEND_TYPE TYPE_ASYNC
#endif
#ifdef BLOCKINGRECV
#define RECV_TYPE TYPE_SYNC
#else
#define RECV_TYPE TYPE_ASYNC
#endif

/* Select a probe routine */
#ifdef PROBE
#define PROBE_ROUTINE Probe
#elif defined(IPROBE)
#define PROBE_ROUTINE Iprobe
#elif defined(MPROBE)
#define PROBE_ROUTINE Mprobe
#elif defined(IMPROBE)
#define PROBE_ROUTINE Improbe
#else
#error "No valid probe type specified"
#endif

#define PROBE_ERROR_DUMMY MPI_ERR_DIMS  /* Dummy values used for the tests */
#define RECV_ERROR_DUMMY MPI_ERR_TOPOLOGY

void probetest_init_data(int id, int data_count, int *buff, int sender_rank)
{
    int j;
    if (sender_rank == MPI_PROC_NULL) {
        for (j = 0; j < data_count; j++)
            buff[j] = 2 * (j + id);
    } else {
        for (j = 0; j < data_count; j++)
            buff[j] = j + id;
    }
}

int probetest_check_recv_result(int id, int verify, int *buff, MPI_Message * message,
                                int recv_count, MPI_Status * status,
                                int sender_rank, int send_count, int tag)
{
    int errs = 0;
    int j;

    /* check data values */
    if (sender_rank == MPI_PROC_NULL) {
        for (j = 0; j < recv_count; j++) {
            if (verify && buff[j] != 2 * (j + id)) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected data %d but got %d\n", id, 2 * (j + id),
                            buff[j]);
            }
        }
    } else {
        for (j = 0; j < recv_count; j++) {
            if (verify && buff[j] != j + id) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected data %d but got %d\n", id, j + id,
                            buff[j]);
            }
        }
    }

    /* check message matching */
    if (PROBE_ROUTINE == Mprobe || PROBE_ROUTINE == Improbe) {
        if (sender_rank == MPI_PROC_NULL) {
            RECORD_ERROR(*message != MPI_MESSAGE_NULL);
            RECORD_ERROR(status->MPI_SOURCE != MPI_PROC_NULL);
            RECORD_ERROR(status->MPI_TAG != MPI_ANY_TAG);
            RECORD_ERROR(status->MPI_ERROR != RECV_ERROR_DUMMY);
        } else {
            RECORD_ERROR(*message != MPI_MESSAGE_NULL); /* expect null after recv */
            RECORD_ERROR(status->MPI_SOURCE != sender_rank);
            RECORD_ERROR(status->MPI_TAG != tag);
            RECORD_ERROR(status->MPI_ERROR != RECV_ERROR_DUMMY);
        }
    }

    return errs;
}

int probetest_invoke_send(int *buff, int data_count, int recv_rank, MPI_Comm comm,
                          int tag, MPI_Request * request)
{
    if (SEND_TYPE == TYPE_SYNC)
        SEND_FUN(buff, data_count, MPI_INT, recv_rank, tag, comm);
    else
        ISEND_FUN(buff, data_count, MPI_INT, recv_rank, tag, comm, request);

    return 0;
}

int probetest_invoke_probe(int sender_rank, int tag, MPI_Comm comm, int *recv_count,
                           MPI_Message * message, MPI_Status * status)
{
    int errs = 0;
    int flag = 0;

    switch (PROBE_ROUTINE) {
        case Probe:
            MPI_Probe(sender_rank, tag, comm, status);
            break;
        case Iprobe:
            while (!flag) {
                MPI_Iprobe(sender_rank, tag, comm, &flag, status);
            }
            break;
        case Mprobe:
            MPI_Mprobe(sender_rank, tag, comm, message, status);
            break;
        case Improbe:
            while (!flag) {
                RECORD_ERROR(*message != MPI_REQUEST_NULL);
                MPI_Improbe(sender_rank, tag, comm, &flag, message, status);
            }
            break;
        default:
            fprintf(stderr, "thread: %d: Probe type not defined \n");
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Find data count */
    MPI_Get_count(status, MPI_INT, recv_count);

    return errs;
}

int probetest_check_probe_result(int id, int *buff, int send_count, int recv_count,
                                 MPI_Message * message, MPI_Status * status, int sender_rank,
                                 int tag)
{
    int errs = 0;
    int j;

    if (sender_rank == MPI_PROC_NULL) {
        RECORD_ERROR(recv_count != 0);  /* expect count 0 for NULL_PROC */
        RECORD_ERROR(status->MPI_SOURCE != MPI_PROC_NULL);
        RECORD_ERROR(status->MPI_TAG != MPI_ANY_TAG);
        RECORD_ERROR(*message != MPI_MESSAGE_NULL);
    } else {
        RECORD_ERROR(recv_count != send_count);
        RECORD_ERROR(status->MPI_SOURCE != sender_rank);
        RECORD_ERROR(status->MPI_TAG != tag);
    }
    if (PROBE_ROUTINE == Mprobe || PROBE_ROUTINE == Improbe)
        RECORD_ERROR(*message == MPI_MESSAGE_NULL);
    RECORD_ERROR(status->MPI_ERROR != PROBE_ERROR_DUMMY);
    return errs;
}

int probetest_invoke_recv(int *buff, int recv_count, int sender_rank, int tag,
                          MPI_Comm comm, MPI_Message * message, MPI_Status * status)
{
    if (PROBE_ROUTINE == Probe || PROBE_ROUTINE == Iprobe) {
        if (RECV_TYPE == TYPE_SYNC) {
            MPI_Recv(buff, recv_count, MPI_INT, sender_rank, tag, comm, MPI_STATUS_IGNORE);
        } else {
            MPI_Request request;
            MPI_Irecv(buff, recv_count, MPI_INT, sender_rank, tag, comm, &request);
            MPI_Wait(&request, status);
        }
    } else {    /* for Mprobe or Improbe */
        if (RECV_TYPE == TYPE_SYNC) {
            MPI_Mrecv(buff, recv_count, MPI_INT, message, status);
        } else {
            MPI_Request request;
            MPI_Imrecv(buff, recv_count, MPI_INT, message, &request);
            MPI_Wait(&request, status);
        }
    }
    return 0;
}

int probetest_send(int id, int send_count, int sender_rank,
                   int *buff, MPI_Comm comm, int tag, int verify, MPI_Request * request)
{
    int errs = 0;
    /* initialize data in send buffer */
    probetest_init_data(id, send_count, buff, sender_rank);
    /* send data */
    errs += probetest_invoke_send(buff, send_count, 1, comm, tag, request);
    return errs;
}

int probetest_probe_n_recv(int id, int send_count, int sender_rank,
                           int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0;
    /* initialize data before recv */
    int recv_count = -1;
    MPI_Message message = MPI_MESSAGE_NULL;
    MPI_Status status, status2;
    status.MPI_ERROR = PROBE_ERROR_DUMMY;
    status2.MPI_ERROR = RECV_ERROR_DUMMY;
    /* call a probe routine */
    errs += probetest_invoke_probe(sender_rank, tag, comm, &recv_count, &message, &status);
    /* initialize recv buffer values */
    probetest_init_data(id, send_count, buff, sender_rank);
    /* call a recv routine */
    errs += probetest_invoke_recv(buff, recv_count, sender_rank, tag, comm, &message, &status2);
    /* check received results */
    errs +=
        probetest_check_recv_result(id, verify, buff, &message, recv_count, &status2,
                                    sender_rank, send_count, tag);
    return errs;
}

int test_probe_normal(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    int sender_rank = 0;
    if (rank == sender_rank) {  /* sender */
        MPI_Request requests[ITER];
        MPI_Status statuses[ITER];
        for (i = 0; i < ITER; i++)
            errs = probetest_send(id, 1, sender_rank, &buff[i], comm, tag, verify, &requests[i]);
        if (SEND_TYPE == TYPE_ASYNC)
            MPI_Waitall(ITER, requests, statuses);
    } else {    /* receiver */
        /* Run tests multiple times */
        for (i = 0; i < ITER; i++) {
            /* Use sender rank 0 and buffer buff[i] */
            errs = probetest_probe_n_recv(id, 1, sender_rank, &buff[i], comm, tag, verify);
        }
    }
    return errs;
}

int test_probe_huge(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    int sender_rank = 0;
    if (rank == sender_rank) {  /* sender */
        MPI_Request request;
        MPI_Status status;
        for (i = 0; i < ITER; i++) {
            errs = probetest_send(id, COUNT, sender_rank, buff, comm, tag, verify, &request);
            /* complete a request before starting another since buff is common across them */
            if (SEND_TYPE == TYPE_ASYNC)
                MPI_Wait(&request, &status);
        }
    } else {    /* receiver */
        /* Run tests multiple times */
        for (i = 0; i < ITER; i++) {
            /* Use sender rank 0 and buffer buff[i] */
            errs = probetest_probe_n_recv(id, COUNT, sender_rank, buff, comm, tag, verify);
        }
    }
    return errs;
}

int test_probe_nullproc(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    int sender_rank = MPI_PROC_NULL;
    /* All ranks recv only */
    for (i = 0; i < ITER; i++) {
        /* Use sender rank 0 and buffer buff[i] */
        errs = probetest_probe_n_recv(id, COUNT, sender_rank, buff, comm, tag, verify);
    }
    return errs;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;

/* Probe tests */
#if defined(HUGE_COUNT) || defined(MEDIUM_COUNT)
    p->result = test_probe_huge(p->id, p->buff, p->comm, p->tag, p->verify);
#else
    p->result = test_probe_normal(p->id, p->buff, p->comm, p->tag, p->verify);
    p->result += test_probe_nullproc(p->id, p->buff, p->comm, p->tag, p->verify);
#endif

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}
