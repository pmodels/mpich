/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

int probetest_check_valid_data(const MPI_Message * message, MPI_Status * status, int sender_rank,
                               int tag);
int probetest_invoke_send(int *buff, int data_count, MPI_Datatype type, int recv_rank,
                          MPI_Comm comm, int tag, MPI_Request * request);
int probetest_invoke_probe(int sender_rank, int tag, MPI_Comm comm, int *recv_count,
                           MPI_Datatype type, MPI_Message * message, MPI_Status * status);
int probetest_check_probe_result(int id, int *buff, int send_count, int recv_count,
                                 MPI_Message * message, MPI_Status * status, int sender_rank,
                                 int tag);
int probetest_invoke_recv(int *buff, int recv_count, MPI_Datatype type, int sender_rank, int tag,
                          MPI_Comm comm, MPI_Message * message, MPI_Status * status);
int probetest_probe_n_recv(int id, int send_count, MPI_Datatype type, int sender_rank,
                           int *buff, MPI_Comm comm, int tag);
int test_probe_normal(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify);
int test_probe_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify);
int test_probe_nullproc(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify);
MTEST_THREAD_RETURN_TYPE run_test(void *arg);

int probetest_check_valid_data(const MPI_Message * message, MPI_Status * status, int sender_rank,
                               int tag)
{
    int errs = 0;

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

int probetest_invoke_send(int *buff, int data_count, MPI_Datatype type, int recv_rank,
                          MPI_Comm comm, int tag, MPI_Request * request)
{
    if (SEND_TYPE == TYPE_SYNC)
        SEND_FUN(buff, data_count, type, recv_rank, tag, comm);
    else
        ISEND_FUN(buff, data_count, type, recv_rank, tag, comm, request);

    return 0;
}

int probetest_invoke_probe(int sender_rank, int tag, MPI_Comm comm, int *recv_count,
                           MPI_Datatype type, MPI_Message * message, MPI_Status * status)
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
            fprintf(stderr, "Probe type not defined \n");
            MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Find data count */
    MPI_Get_count(status, type, recv_count);

    return errs;
}

int probetest_check_probe_result(int id, int *buff, int send_count, int recv_count,
                                 MPI_Message * message, MPI_Status * status, int sender_rank,
                                 int tag)
{
    int errs = 0;

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

int probetest_invoke_recv(int *buff, int recv_count, MPI_Datatype type, int sender_rank, int tag,
                          MPI_Comm comm, MPI_Message * message, MPI_Status * status)
{
    if (PROBE_ROUTINE == Probe || PROBE_ROUTINE == Iprobe) {
        if (RECV_TYPE == TYPE_SYNC) {
            MPI_Recv(buff, recv_count, type, sender_rank, tag, comm, MPI_STATUS_IGNORE);
        } else {
            MPI_Request request;
            MPI_Irecv(buff, recv_count, type, sender_rank, tag, comm, &request);
            MPI_Wait(&request, status);
        }
    } else {    /* for Mprobe or Improbe */
        if (RECV_TYPE == TYPE_SYNC) {
            MPI_Mrecv(buff, recv_count, type, message, status);
        } else {
            MPI_Request request;
            MPI_Imrecv(buff, recv_count, type, message, &request);
            MPI_Wait(&request, status);
        }
    }
    return 0;
}

int probetest_probe_n_recv(int id, int send_count, MPI_Datatype type, int sender_rank,
                           int *buff, MPI_Comm comm, int tag)
{
    int errs = 0;
    /* initialize data before recv */
    int recv_count = -1;
    MPI_Message message = MPI_MESSAGE_NULL;
    MPI_Status status, status2;
    status.MPI_ERROR = PROBE_ERROR_DUMMY;
    status2.MPI_ERROR = RECV_ERROR_DUMMY;
    /* call a probe routine */
    errs += probetest_invoke_probe(sender_rank, tag, comm, &recv_count, type, &message, &status);
    /* call a recv routine */
    errs +=
        probetest_invoke_recv(buff, recv_count, type, sender_rank, tag, comm, &message, &status2);
    /* check received results */
    errs += probetest_check_valid_data(&message, &status2, sender_rank, tag);
    return errs;
}

int test_probe_normal(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {    /* sender */
        MPI_Request *requests = malloc(sizeof(MPI_Request) * iter);
        for (i = 0; i < iter; i++) {
            buff[i] = SETVAL(i, id);
            errs = probetest_invoke_send(&buff[i], 1, type, 1, comm, tag, &requests[i]);
        }
        if (SEND_TYPE == TYPE_ASYNC)
            MPI_Waitall(iter, requests, MPI_STATUSES_IGNORE);
        free(requests);
    } else {    /* receiver */
        /* Run tests multiple times */
        for (i = 0; i < iter; i++) {
            /* Use sender rank 0 and buffer buff[i] */
            buff[i] = -1;
            errs += probetest_probe_n_recv(id, 1, type, 0 /* sender */ , &buff[i], comm, tag);
            if (verify && buff[i] != SETVAL(i, id)) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected %d but got %d\n", id, SETVAL(i, id),
                            buff[i]);
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);

    }

    return errs;
}

int test_probe_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, rank;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {    /* sender */
        MPI_Request request;
        for (int i = 0; i < count; i++)
            buff[i] = SETVAL(i, id);
        for (int i = 0; i < iter; i++) {
            errs = probetest_invoke_send(buff, count, type, 1, comm, tag, &request);
            /* complete a request before starting another since buff is common across them */
            if (SEND_TYPE == TYPE_ASYNC)
                MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
    } else {    /* receiver */
        /* Run tests multiple times */
        for (int i = 0; i < iter; i++) {
            for (int j = 0; j < count; j++)
                buff[j] = -1;
            errs += probetest_probe_n_recv(id, count, type, 0 /* sender_rank */ , buff, comm,
                                           tag);
            if (verify) {
                for (int j = 0; j < count; j++) {
                    if (buff[j] != SETVAL(j, id)) {
                        errs++;
                        if (errs <= DATA_WARN_THRESHOLD)
                            fprintf(stderr, "thread %d: Expected %d but got %d\n", id,
                                    SETVAL(j, id), buff[j]);
                    }
                }
            }

        }
    }

  fn_exit:
    return errs;
}

int test_probe_nullproc(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    int sender_rank = MPI_PROC_NULL;
    /* All ranks recv only */
    for (i = 0; i < iter; i++) {
        errs = probetest_probe_n_recv(id, count, type, sender_rank, buff, comm, tag);
    }

  fn_exit:
    return errs;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;

/* Probe tests */
#if defined(HUGE_COUNT)
    p->result = test_probe_huge(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);
#else
    MTestPrintfMsg(2, "Testing probe_normal\n");
    p->result = test_probe_normal(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);

    /* Barrier across threads. */
    MTest_thread_barrier(NTHREADS);
    if (p->id == 0) {
        MPI_Barrier(p->comm);
    }
    MTest_thread_barrier(NTHREADS);

    MTestPrintfMsg(2, "Testing probe_nullproc\n");
    p->result += test_probe_nullproc(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);
#endif

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}
