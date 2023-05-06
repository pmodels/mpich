/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM
#include "threadcomm_pt2pt.h"

bool MPIR_threadcomm_was_thread_single;
UT_icd MPIR_threadcomm_icd = { sizeof(MPIR_threadcomm_tls_t), NULL, NULL, NULL };

MPL_TLS UT_array *MPIR_threadcomm_array = NULL;

int MPIR_Threadcomm_init_impl(MPIR_Comm * comm, int num_threads, MPIR_Comm ** comm_out)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size = comm->local_size;

    /* we will modify MPIR_ThreadInfo.isThreaded in MPIR_Threadcomm_start */
    MPIR_threadcomm_was_thread_single = !MPIR_ThreadInfo.isThreaded;

    MPIR_Threadcomm *threadcomm;
    threadcomm = MPL_malloc(sizeof(MPIR_Threadcomm), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIR_Comm *dup_comm;
    mpi_errno = MPIR_Comm_dup_impl(comm, &dup_comm);
    MPIR_ERR_CHECK(mpi_errno);
    dup_comm->threadcomm = threadcomm;

    int *threads_table;
    threads_table = MPL_malloc(comm_size * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threads_table, mpi_errno, MPI_ERR_OTHER, "**nomem");

    mpi_errno = MPIR_Allgather_impl(&num_threads, 1, MPI_INT, threads_table, 1, MPI_INT, comm,
                                    MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    int *rank_offset_table;;
    rank_offset_table = MPL_malloc(comm_size * sizeof(int), MPL_MEM_OTHER);

    int offset = 0;
    for (int i = 0; i < comm_size; i++) {
        offset += threads_table[i];
        rank_offset_table[i] = offset;
    }

    threadcomm->kind = MPIR_THREADCOMM_KIND__PERSIST;
    threadcomm->comm = dup_comm;
    threadcomm->num_threads = num_threads;
    threadcomm->rank_offset_table = rank_offset_table;

    MPL_atomic_relaxed_store_int(&threadcomm->arrive_counter, 0);
    MPL_atomic_relaxed_store_int(&threadcomm->leave_counter, num_threads);
    MPL_atomic_relaxed_store_int(&threadcomm->barrier_flag, 0);

#if MPIR_THREADCOMM_TRANSPORT == MPIR_THREADCOMM_USE_FBOX
    threadcomm->mailboxes = MPL_calloc(num_threads * num_threads, MPIR_THREADCOMM_FBOX_SIZE,
                                       MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm->mailboxes, mpi_errno, MPI_ERR_OTHER, "**nomem");
#elif MPIR_THREADCOMM_TRANSPORT == MPIR_THREADCOMM_USE_QUEUE
    threadcomm->queues = MPL_calloc(num_threads, sizeof(MPIR_threadcomm_queue_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm->queues, mpi_errno, MPI_ERR_OTHER, "**nomem");
#endif

    MPL_free(threads_table);

  fn_exit:
    *comm_out = dup_comm;
    return mpi_errno;
  fn_fail:
    dup_comm = NULL;
    goto fn_exit;
}

int MPIR_Threadcomm_free_impl(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    if (threadcomm->kind == MPIR_THREADCOMM_KIND__DERIVED) {
        /* duplicated threadcomm are freed inside the parallel region using MPI_Comm_free */
        MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);
        MPIR_Assert(p);

        mpi_errno = MPIR_Threadcomm_finish_impl(comm);
        MPIR_ERR_CHECK(mpi_errno);

        if (p->tid > 0) {
            goto fn_exit;
        }
    }

    /* make sure MPIR_Comm_free_impl do not invoke the threadcomm paths */
    comm->threadcomm = NULL;

    mpi_errno = MPIR_Comm_free_impl(comm);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(threadcomm->rank_offset_table);

#if MPIR_THREADCOMM_TRANSPORT == MPIR_THREADCOMM_USE_FBOX
    MPL_free(threadcomm->mailboxes);
#elif MPIR_THREADCOMM_TRANSPORT == MPIR_THREADCOMM_USE_QUEUE
    MPL_free(threadcomm->queues);
#endif

    MPL_free(threadcomm);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Threadcomm_{start,finish} require thread barrier to consistently
 * modify global settings
 */
#define MPIR_THREACOMM_ACTION_NONE   0
#define MPIR_THREACOMM_ACTION_START  1
#define MPIR_THREACOMM_ACTION_FINISH 2

/* Returns arrive id.
 * Do action after every threads arrive and before first thread leave.
 */
static int thread_barrier(MPIR_Threadcomm * threadcomm, int action)
{
    /* A thread barrier */
    int P = threadcomm->num_threads;

    if (MPL_atomic_load_int(&threadcomm->arrive_counter) == 0) {
        /* potentially some are still in previous barrier, wait for it */
        while (MPL_atomic_load_int(&threadcomm->leave_counter) < P) {
        }
        MPL_atomic_store_int(&(threadcomm->barrier_flag), 0);
    }

    int arrive_id = MPL_atomic_fetch_add_int(&threadcomm->arrive_counter, 1);

    if (arrive_id == P - 1) {
        /* the last one resets flags */
        MPL_atomic_store_int(&threadcomm->arrive_counter, 0);

        /* This is the 1st thread to leave. Modify global settings here.
         * It is critical that we sandwidch between setting
         * arrive_counter and leave_counter.
         */
        if (action == MPIR_THREACOMM_ACTION_START) {
            if (MPIR_threadcomm_was_thread_single) {
                MPIR_ThreadInfo.isThreaded = 1;
            }
        } else if (action == MPIR_THREACOMM_ACTION_FINISH) {
            if (MPIR_threadcomm_was_thread_single) {
                MPIR_ThreadInfo.isThreaded = 0;
            }
        }

        MPL_atomic_store_int(&threadcomm->leave_counter, 1);
        MPL_atomic_store_int(&threadcomm->barrier_flag, 1);
    } else {
        /* every one else wait for the last one */
        while (!MPL_atomic_load_int(&threadcomm->barrier_flag)) {
        }
        MPL_atomic_fetch_add_int(&threadcomm->leave_counter, 1);
    }

    return arrive_id;
}

/* NOTE: we are adding thread_barrier in both MPIX_Threadcomm_{start,finish}
 *       to ensure global settings (e.g. isThreaded) are modified effectively.
 */

/* threadcomm need MPIR_ThreadInfo.isThreaded on to enable critical sections,
 * we can turn it on or off at MPIX_Threadcomm_{start,finish}. This will work
 * since outside the parallel region all MPI access are thread single/serialized.
 */

int MPIR_Threadcomm_start_impl(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    int tid = thread_barrier(threadcomm, MPIR_THREACOMM_ACTION_START);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Assert(MPIR_ThreadInfo.isThreaded);
    /* memory allocation may depend on isThreaded, e.g. when memtrace
     * is enabled, thus make sure it is after a thread_barrier.
     */
    MPIR_threadcomm_tls_t *p;
    MPIR_THREADCOMM_TLS_ADD(threadcomm, p);

    MPIR_Assert(p);
    p->tid = tid;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_finish_impl(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Threadcomm *threadcomm = comm->threadcomm;
    MPIR_Assert(threadcomm);
    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(comm->threadcomm);
    MPIR_Assert(p);

    if (MPIR_Process.attr_free && p->attributes) {
        mpi_errno = MPIR_Process.attr_free(comm->handle, &p->attributes);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_THREADCOMM_TLS_DELETE(threadcomm);

    MPIR_Assert(MPIR_ThreadInfo.isThreaded);

    thread_barrier(threadcomm, MPIR_THREACOMM_ACTION_FINISH);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Threadcomm_size_impl(MPIR_Comm * comm, int *size)
{
    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    int comm_size = comm->local_size;
    *size = threadcomm->rank_offset_table[comm_size - 1];
    return MPI_SUCCESS;
}

int MPIR_Threadcomm_rank_impl(MPIR_Comm * comm, int *rank)
{
    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    *rank = MPIR_THREADCOMM_TID_TO_RANK(threadcomm, MPIR_threadcomm_get_tid(threadcomm));
    return MPI_SUCCESS;
}

int MPIR_Threadcomm_dup_impl(MPIR_Comm * comm, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);

    MPIR_Threadcomm *threadcomm = comm->threadcomm;
    int num_threads = threadcomm->num_threads;
    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);
    MPIR_Assert(p);

    MPIR_Comm *newcomm = NULL;
    if (p->tid == 0) {
        mpi_errno = MPIR_Threadcomm_init_impl(comm, num_threads, &newcomm);
        newcomm->threadcomm->kind = MPIR_THREADCOMM_KIND__DERIVED;
        MPIR_ERR_CHECK(mpi_errno);
        /* bcast to all threads */
        threadcomm->bcast_value = newcomm;
    }
    thread_barrier(threadcomm, MPIR_THREACOMM_ACTION_NONE);
    if (p->tid > 0) {
        newcomm = threadcomm->bcast_value;
    }

    mpi_errno = MPIR_Threadcomm_start_impl(newcomm);
    MPIR_ERR_CHECK(mpi_errno);

    *newcomm_ptr = newcomm;

  fn_exit:
    return mpi_errno;
  fn_fail:
    *newcomm_ptr = NULL;
    goto fn_exit;
}

#else /* ! ENABLE_THREADCOMM */
int MPIR_Threadcomm_init_impl(MPIR_Comm * comm, int num_threads, MPIR_Comm ** comm_out)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIR_Threadcomm_free_impl(MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIR_Threadcomm_start_impl(MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIR_Threadcomm_finish_impl(MPIR_Comm * comm)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* ENABLE_THREADCOMM */
