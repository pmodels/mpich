/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpir_async_things.h"

static struct MPIR_Async_thing *async_things_list[MPIR_MAX_VCIS + 1];
static MPID_Thread_mutex_t async_things_mutex[MPIR_MAX_VCIS + 1];
static int async_things_progress_hook_id[MPIR_MAX_VCIS + 1];

int MPIR_Async_things_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        int err;
        MPID_Thread_mutex_create(&async_things_mutex[vci], &err);
        MPIR_Assert(err == 0);

        /* vci == MPIR_MAX_VCIS is equivalent to -1 */
        mpi_errno = MPIR_Progress_hook_register(vci, MPIR_Async_things_progress,
                                                &async_things_progress_hook_id[vci]);
    }
    return mpi_errno;
}

int MPIR_Async_things_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        while (async_things_list[vci] != NULL) {
            PMPIX_Stream_progress(MPIX_STREAM_NULL);
        }
    }

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        int err;
        MPID_Thread_mutex_destroy(&async_things_mutex[vci], &err);
        MPIR_Assert(err == 0);

        mpi_errno = MPIR_Progress_hook_deregister(async_things_progress_hook_id[vci]);
    }

    return mpi_errno;
}

int MPIR_Async_things_add(int (*poll_fn) (struct MPIR_Async_thing * entry), void *state,
                          MPIR_Stream * stream_ptr)
{
    struct MPIR_Async_thing *entry = MPL_malloc(sizeof(struct MPIR_Async_thing), MPL_MEM_OTHER);
    entry->poll_fn = poll_fn;
    entry->state = state;
    entry->stream_ptr = stream_ptr;
    entry->new_entries = NULL;

    int vci = MPIR_MAX_VCIS;
    if (stream_ptr) {
        vci = stream_ptr->vci;
    }

    MPID_THREAD_CS_ENTER(VCI, async_things_mutex[vci]);
    bool was_empty = (async_things_list[vci] == NULL);
    DL_APPEND(async_things_list[vci], entry);
    MPID_THREAD_CS_EXIT(VCI, async_things_mutex[vci]);

    if (was_empty) {
        MPIR_Progress_hook_activate(async_things_progress_hook_id[vci]);
    }

    return MPI_SUCCESS;
}

int MPIR_Async_things_progress(int vci, int *made_progress)
{
    if (vci == -1) {
        vci = MPIR_MAX_VCIS;
    }
    struct MPIR_Async_thing *entry, *tmp;
    MPID_THREAD_CS_ENTER(VCI, async_things_mutex[vci]);
    DL_FOREACH_SAFE(async_things_list[vci], entry, tmp) {
        int ret = entry->poll_fn(entry);
        if (entry->new_entries) {
            *made_progress = 1;
            DL_CONCAT(async_things_list[vci], entry->new_entries);
            entry->new_entries = NULL;
        }
        if (ret == MPIX_ASYNC_DONE) {
            *made_progress = 1;
            DL_DELETE(async_things_list[vci], entry);
            MPL_free(entry);
            if (async_things_list[vci] == NULL) {
                MPIR_Progress_hook_deactivate(async_things_progress_hook_id[vci]);
            }
        }
    }
    MPID_THREAD_CS_EXIT(VCI, async_things_mutex[vci]);
    return MPI_SUCCESS;
}
