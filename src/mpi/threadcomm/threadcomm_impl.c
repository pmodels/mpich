/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "threadcomm_pt2pt.h"

#ifdef ENABLE_THREADCOMM

UT_icd MPIR_threadcomm_icd = { sizeof(MPIR_threadcomm_tls_t), NULL, NULL, NULL };

MPL_TLS UT_array *MPIR_threadcomm_array = NULL;

int MPIR_Threadcomm_init_impl(MPIR_Comm * comm, int num_threads, MPIR_Comm ** comm_out)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size = comm->local_size;

    /* When each thread use different request pool, we don't necessarily need
     * MPI_THREAD_MULTIPLE for lock safety. But check when num_threads exceed
     * MPIR_REQUEST_NUM_POOLS.
     */
    if (num_threads > MPIR_REQUEST_NUM_POOLS) {
        MPIR_ERR_CHKANDJUMP(!MPIR_ThreadInfo.isThreaded, mpi_errno, MPI_ERR_OTHER,
                            "**threadcomm_threadlevel");
    }

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

    threadcomm->in_counters = MPL_calloc(num_threads * num_threads, MPL_CACHELINE_SIZE,
                                         MPL_MEM_OTHER);

    MPL_atomic_relaxed_store_int(&threadcomm->next_id, 0);
    MPL_atomic_relaxed_store_int(&threadcomm->arrive_counter, 0);
    MPL_atomic_relaxed_store_int(&threadcomm->leave_counter, num_threads);
    MPL_atomic_relaxed_store_int(&threadcomm->barrier_flag, 0);

#ifdef MPIR_THREADCOMM_USE_FBOX
    threadcomm->mailboxes = MPL_calloc(num_threads * num_threads, MPIR_THREADCOMM_FBOX_SIZE,
                                       MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm->mailboxes, mpi_errno, MPI_ERR_OTHER, "**nomem");
#else
    threadcomm->queues = MPL_calloc(num_threads, sizeof(MPIR_threadcomm_queue_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm->queues, mpi_errno, MPI_ERR_OTHER, "**nomem");

    threadcomm->pools = MPL_calloc(num_threads, sizeof(MPIR_threadcomm_queue_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!threadcomm->pools, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPI_Aint slab_sz = MPIR_THREADCOMM_CELL_SIZE * 1024 * num_threads;
    threadcomm->cell_slab = MPL_malloc(slab_sz, MPL_MEM_OTHER);
    for (int i = 0; i < num_threads; i++) {
        char *slab = threadcomm->cell_slab + i * MPIR_THREADCOMM_CELL_SIZE * 1024;
        for (int j = 0; j < 1024; j++) {
            threadcomm_mpsc_enqueue(&threadcomm->pools[i],
                                    (void *) slab + j * MPIR_THREADCOMM_CELL_SIZE);
        }
    }
#endif

    MPL_free(threads_table);

  fn_exit:
    *comm_out = dup_comm;
    return mpi_errno;
  fn_fail:
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

    MPL_free(threadcomm->in_counters);

    MPL_free(threadcomm->rank_offset_table);
#ifdef MPIR_THREADCOMM_USE_FBOX
    MPL_free(threadcomm->mailboxes);
#else
    MPL_free(threadcomm->queues);
    MPL_free(threadcomm->pools);
    MPL_free(threadcomm->cell_slab);
#endif

    MPL_free(threadcomm);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* FIXME: this can be inefficient due to multiple threads spin on the same var.
 *        We should try the dissemination algorithm. */
static int thread_barrier(MPIR_Threadcomm * threadcomm)
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
        MPL_atomic_store_int(&threadcomm->leave_counter, 1);
        MPL_atomic_store_int(&threadcomm->barrier_flag, 1);
    } else {
        /* every one else wait for the last one */
        while (!MPL_atomic_load_int(&threadcomm->barrier_flag)) {
        }
        MPL_atomic_fetch_add_int(&threadcomm->leave_counter, 1);
    }

    return MPI_SUCCESS;
}

/* MPIR_Threadcomm_{start,finish} called inside parallel region by every thread.
 * threadcomm->next_id can be used to test whether it is active (started).
 */
int MPIR_Threadcomm_start_impl(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm->threadcomm);
    MPIR_Threadcomm *threadcomm = comm->threadcomm;

    MPIR_threadcomm_tls_t *p;
    MPIR_THREADCOMM_TLS_ADD(threadcomm, p);

    MPIR_Assert(p);
    p->tid = MPL_atomic_fetch_add_int(&threadcomm->next_id, 1);

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
    MPIR_threadcomm_tls_t *tls = MPIR_threadcomm_get_tls(comm->threadcomm);
    MPIR_Assert(tls);

    mpi_errno = thread_barrier(threadcomm);
    MPIR_ERR_CHECK(mpi_errno);

    if (tls->tid == 0) {
        /* reset next_id and a barrier to ensure next MPI_Threadcomm_start to work */
        MPL_atomic_store_int(&threadcomm->next_id, 0);
    }

    if (MPIR_Process.attr_free && tls->attributes) {
        mpi_errno = MPIR_Process.attr_free(comm->handle, &tls->attributes);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_THREADCOMM_TLS_DELETE(threadcomm);

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
    mpi_errno = thread_barrier(threadcomm);
    MPIR_ERR_CHECK(mpi_errno);
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
