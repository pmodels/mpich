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

    - name        : MPIR_CVAR_CH4_PROGRESS_THREAD_AFFINITY
      category    : CH4
      type        : string
      default     : ""
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specifies affinity for all progress threads of local processes.
        Format - comma-separated list of logical processors. In case of
        N progress threads per process, the first N logical processors
        from list will be assigned to threads of first local process.
        The next N logical processors from list - to second local process
        and so on. For example, thread affinity is "0,1,2,3", 2 progress
        threads per process and 2 processes per node. Progress threads
        of first local process will be pinned on logical processors "0,1",
        progress threads of second local process - on "2,3".

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#if defined(MPICH_IS_THREADED) && MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE

static int MPIR_async_thread_initialized = 0;
static MPID_Thread_id_t progress_thread_id;
static MPL_atomic_int_t async_done = MPL_ATOMIC_INT_T_INITIALIZER(0);

static void progress_fn(void *data)
{
    MPID_Progress_state state;

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    MPID_Progress_start(&state);
    while (MPL_atomic_load_int(&async_done) == 0) {
        MPID_Progress_test(&state);
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
    MPID_Progress_end(&state);

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_parse_progress_thread_affinity(int *thread_affinity,
                                                                  int async_threads_per_node)
{
    int th_idx, read_count = 0, mpi_errno = MPI_SUCCESS;
    char *affinity_copy = NULL;
    const char *affinity_to_parse = MPIR_CVAR_CH4_PROGRESS_THREAD_AFFINITY;
    char *proc_id_str, *tmp;
    size_t proc_count;

    if (!affinity_to_parse || strlen(affinity_to_parse) == 0) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**parse_thread_affinity",
                             "**parse_thread_affinity %s", affinity_to_parse);
    }

    /* create copy of original buffer because it will be modified in strsep */
    affinity_copy = MPL_strdup(affinity_to_parse);
    MPIR_Assert(affinity_copy);
    tmp = affinity_copy;

    for (th_idx = 0; th_idx < async_threads_per_node; th_idx++) {
        proc_id_str = MPL_strsep(&tmp, ",");
        if (proc_id_str != NULL) {
            if (strlen(proc_id_str) == 0 || (strlen(proc_id_str) > 0 && !isdigit(proc_id_str[0])) ||
                atoi(proc_id_str) < 0) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**parse_thread_affinity",
                                     "**parse_thread_affinity %s", affinity_to_parse);
            }
            thread_affinity[th_idx] = atoi(proc_id_str);
            read_count++;
        } else {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**parse_thread_affinity",
                                 "**parse_thread_affinity %s", affinity_to_parse);
        }
    }
    if (read_count < async_threads_per_node) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**parse_thread_affinity",
                             "**parse_thread_affinity %s", affinity_to_parse);
    }

  fn_exit:
    MPL_free(affinity_copy);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int get_thread_affinity(bool * apply_affinity, int **p_thread_affinity, int *affinity_idx)
{
    int mpi_errno = MPI_SUCCESS;
    int global_rank, local_rank, local_size, async_threads_per_node;
    int *thread_affinity = NULL;
    *apply_affinity = MPIR_CVAR_CH4_PROGRESS_THREAD_AFFINITY &&
        strlen(MPIR_CVAR_CH4_PROGRESS_THREAD_AFFINITY) > 0;

    if (*apply_affinity) {
        global_rank = MPIR_Process.rank;
        local_rank =
            (MPIR_Process.comm_world->node_comm) ? MPIR_Process.comm_world->node_comm->rank : 0;
        local_size =
            (MPIR_Process.comm_world->node_comm) ? MPIR_Process.comm_world->
            node_comm->local_size : 1;
        async_threads_per_node = local_size;
        thread_affinity = (int *) MPL_malloc(async_threads_per_node * sizeof(int), MPL_MEM_OTHER);

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST,
                         " global_rank %d, local_rank %d, local_size %d, async_threads_per_node %d",
                         global_rank, local_rank, local_size, async_threads_per_node));

        mpi_errno = MPIDI_parse_progress_thread_affinity(thread_affinity, async_threads_per_node);
        MPIR_ERR_CHECK(mpi_errno);

        if (MPIR_Process.rank == 0) {
            int th_idx;
            for (th_idx = 0; th_idx < async_threads_per_node; th_idx++) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "affinity: thread %d, processor %d",
                                 th_idx, thread_affinity[th_idx]));
            }
        }
        *p_thread_affinity = thread_affinity;
        *affinty_idx = local_rank;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called inside MPID_Init_async_thread to provide device override */
int MPIR_Init_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS, thr_err;
    int *thread_affinity = NULL, affinity_idx;
    bool apply_affinity;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

    mpi_errno = get_thread_affinity(&apply_affinity, &thread_affinity, &affinity_idx);
    MPIR_ERR_CHECK(mpi_errno);

    int err = 0;
    MPID_Thread_create((MPID_Thread_func_t) progress_fn, NULL, &progress_thread_id, &err);
    MPIR_ERR_CHECK(mpi_errno);

    if (apply_affinity) {
        MPL_thread_set_affinity(progress_thread_id, &(thread_affinity[affinity_idx]), 1, &thr_err);
        MPIR_ERR_CHKANDJUMP1(thr_err, mpi_errno, MPI_ERR_OTHER, "**set_thread_affinity",
                             "**set_thread_affinity %d", thread_affinity[affinity_idx]);
    }

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_INIT_ASYNC_THREAD);

  fn_exit:
    MPL_free(thread_affinity);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called inside MPID_Finalize_async_thread to provide device override */
int MPIR_Finalize_async_thread(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    MPL_atomic_store_int(&async_done, 1);
    MPID_Thread_join(progress_thread_id);

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_FINALIZE_ASYNC_THREAD);

    return mpi_errno;
}

/* called inside MPIR_Init_thread_impl */
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
