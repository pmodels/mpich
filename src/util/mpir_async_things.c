/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpir_async_things.h"

static struct MPIR_Async_thing *async_things_list;
static MPID_Thread_mutex_t async_things_mutex;
static int async_things_progress_hook_id;

int MPIR_Async_things_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    int err;
    MPID_Thread_mutex_create(&async_things_mutex, &err);
    MPIR_Assert(err == 0);

    mpi_errno = MPIR_Progress_hook_register(-1, MPIR_Async_things_progress,
                                            &async_things_progress_hook_id);
    return mpi_errno;
}

int MPIR_Async_things_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    while (async_things_list != NULL) {
        PMPIX_Stream_progress(MPIX_STREAM_NULL);
    }

    int err;
    MPID_Thread_mutex_destroy(&async_things_mutex, &err);
    MPIR_Assert(err == 0);

    mpi_errno = MPIR_Progress_hook_deregister(async_things_progress_hook_id);
    return mpi_errno;
}

int MPIR_Async_things_add(int (*poll_fn) (struct MPIR_Async_thing * entry), void *state)
{
    struct MPIR_Async_thing *entry = MPL_malloc(sizeof(struct MPIR_Async_thing), MPL_MEM_OTHER);
    entry->poll_fn = poll_fn;
    entry->state = state;
    entry->new_entries = NULL;

    MPID_THREAD_CS_ENTER(VCI, async_things_mutex);
    bool was_empty = (async_things_list == NULL);
    DL_APPEND(async_things_list, entry);
    MPID_THREAD_CS_EXIT(VCI, async_things_mutex);

    if (was_empty) {
        MPIR_Progress_hook_activate(async_things_progress_hook_id);
    }

    return MPI_SUCCESS;
}

int MPIR_Async_things_progress(int *made_progress)
{
    struct MPIR_Async_thing *entry, *tmp;
    MPID_THREAD_CS_ENTER(VCI, async_things_mutex);
    DL_FOREACH_SAFE(async_things_list, entry, tmp) {
        int ret = entry->poll_fn(entry);
        if (ret != MPIX_ASYNC_NOPROGRESS) {
            *made_progress = 1;
            if (entry->new_entries) {
                DL_CONCAT(async_things_list, entry->new_entries);
                entry->new_entries = NULL;
            }
            if (ret == MPIX_ASYNC_DONE) {
                DL_DELETE(async_things_list, entry);
                MPL_free(entry);
                if (async_things_list == NULL) {
                    MPIR_Progress_hook_deactivate(async_things_progress_hook_id);
                }
            }
        }
    }
    MPID_THREAD_CS_EXIT(VCI, async_things_mutex);
    return MPI_SUCCESS;
}
