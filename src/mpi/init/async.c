/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"
#include "mpiu_thread.h"

#ifndef MPICH_MPI_FROM_PMPI

#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
static MPID_Comm *progress_comm_ptr;
static MPID_Thread_id_t progress_thread_id;
static MPID_Thread_mutex_t progress_mutex;
static MPID_Thread_cond_t progress_cond;
static volatile int progress_thread_done = 0;

/* We can use whatever tag we want; we use a different communicator
 * for communicating with the progress thread. */
#define WAKE_TAG 100

#undef FUNCNAME
#define FUNCNAME progress_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static void progress_fn(void * data)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPI_Request request;
    MPI_Status status;

    /* Explicitly add CS_ENTER/EXIT since this thread is created from
     * within an internal function and will call NMPI functions
     * directly. */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* FIXME: We assume that waiting on some request forces progress
     * on all requests. With fine-grained threads, will this still
     * work as expected? We can imagine an approach where a request on
     * a non-conflicting communicator would not touch the remaining
     * requests to avoid locking issues. Once the fine-grained threads
     * code is fully functional, we need to revisit this and, if
     * appropriate, either change what we do in this thread, or delete
     * this comment. */

    mpi_errno = MPID_Irecv(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm_ptr,
                           MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIU_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait_impl(&request, &status);
    MPIU_Assert(!mpi_errno);

    /* Send a signal to the main thread saying we are done */
    MPID_Thread_mutex_lock(&progress_mutex, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    progress_thread_done = 1;

    MPID_Thread_mutex_unlock(&progress_mutex, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    MPID_Thread_cond_signal(&progress_cond, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return;
}

#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */

#undef FUNCNAME
#define FUNCNAME MPIR_Init_async_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Init_async_thread(void)
{
#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_self_ptr;
    int err = 0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_INIT_ASYNC_THREAD);


    /* Dup comm world for the progress thread */
    MPID_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, &progress_comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPID_Thread_cond_create(&progress_cond, &err);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**cond_create", "**cond_create %s", strerror(err));
    
    MPID_Thread_mutex_create(&progress_mutex, &err);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**mutex_create", "**mutex_create %s", strerror(err));
    
    MPID_Thread_create((MPID_Thread_func_t) progress_fn, NULL, &progress_thread_id, &err);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**mutex_create", "**mutex_create %s", strerror(err));
    
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
#else
    return MPI_SUCCESS;
#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Finalize_async_thread
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Finalize_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS;
#if MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE
    MPID_Request *request_ptr = NULL;
    MPI_Request request;
    MPI_Status status;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    mpi_errno = MPID_Isend(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm_ptr,
                           MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIU_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait_impl(&request, &status);
    MPIU_Assert(!mpi_errno);

    /* XXX DJG why is this unlock/lock necessary?  Should we just YIELD here or later?  */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPID_Thread_mutex_lock(&progress_mutex, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    while (!progress_thread_done) {
        MPID_Thread_cond_wait(&progress_cond, &progress_mutex, &mpi_errno);
        MPIU_Assert(!mpi_errno);
    }

    MPID_Thread_mutex_unlock(&progress_mutex, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    mpi_errno = MPIR_Comm_free_impl(progress_comm_ptr);
    MPIU_Assert(!mpi_errno);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPID_Thread_cond_destroy(&progress_cond, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    MPID_Thread_mutex_destroy(&progress_mutex, &mpi_errno);
    MPIU_Assert(!mpi_errno);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

#endif /* MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE */
    return mpi_errno;
}

#endif
