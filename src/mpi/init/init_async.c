/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ASYNC_PROGRESS
      category    : THREADS
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPICH will initiate an additional thread to
        make asynchronous progress on all communication operations
        including point-to-point, collective, one-sided operations and
        I/O.  Setting this variable will automatically increase the
        thread-safety level to MPI_THREAD_MULTIPLE.  While this
        improves the progress semantics, it might cause a small amount
        of performance overhead for regular MPI operations.  The user
        is encouraged to leave one or more hardware threads vacant in
        order to prevent contention between the application threads
        and the progress thread(s).  The impact of oversubscription is
        highly system dependent but may be substantial in some cases,
        hence this recommendation.


=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#if defined(MPICH_IS_THREADED) && MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE

static int MPIR_async_thread_initialized = 0;

static MPIR_Comm *progress_comm_ptr;
static MPID_Thread_id_t progress_thread_id;

/* We can use whatever tag we want; we use a different communicator
 * for communicating with the progress thread. */
#define WAKE_TAG 100

static void progress_fn(void *data)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr = NULL;
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
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait(&request, &status);
    MPIR_Assert(!mpi_errno);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return;
}

/* called inside MPID_Init_async_thread to provide device override */
int MPIR_Init_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_self_ptr;
    int err = 0;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_INIT_ASYNC_THREAD);


    /* Dup comm world for the progress thread */
    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, NULL, &progress_comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_Thread_create((MPID_Thread_func_t) progress_fn, NULL, &progress_thread_id, &err);
    MPIR_ERR_CHKANDJUMP1(err, mpi_errno, MPI_ERR_OTHER, "**mutex_create", "**mutex_create %s",
                         strerror(err));

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called inside MPID_Finalize_async_thread to provide device override */
int MPIR_Finalize_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr = NULL;
    MPI_Request request;
    MPI_Status status;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    mpi_errno = MPID_Isend(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assert(!mpi_errno);
    request = request_ptr->handle;
    mpi_errno = MPIR_Wait(&request, &status);
    MPIR_Assert(!mpi_errno);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPID_Thread_join(progress_thread_id);

    mpi_errno = MPIR_Comm_free_impl(progress_comm_ptr);
    MPIR_Assert(!mpi_errno);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    return mpi_errno;
}

/* called inside MPIR_Init_thread */
int MPII_init_async(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ASYNC_PROGRESS) {
        if (MPIR_ThreadInfo.thread_provided == MPI_THREAD_MULTIPLE) {
            mpi_errno = MPID_Init_async_thread();
            if (mpi_errno)
                goto fn_fail;

            MPIR_async_thread_initialized = 1;
        } else {
            printf("WARNING: No MPI_THREAD_MULTIPLE support (needed for async progress)\n");
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called inside MPI_Finalize */
int MPII_finalize_async(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user requested for asynchronous progress, we need to
     * shutdown the progress thread */
    if (MPIR_async_thread_initialized) {
        mpi_errno = MPID_Finalize_async_thread();
    }

    return mpi_errno;
}

#else
int MPIR_Finalize_async_thread(void)
{
    return MPI_SUCCESS;
}

int MPIR_Init_async_thread(void)
{
    return MPI_SUCCESS;
}

int MPII_init_async(void)
{
    return MPI_SUCCESS;
}

int MPII_finalize_async(void)
{
    return MPI_SUCCESS;
}
#endif
