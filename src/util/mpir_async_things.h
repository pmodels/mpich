/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_ASYNC_THINGS_H_INCLUDED
#define MPIR_ASYNC_THINGS_H_INCLUDED

/* Async_things is the most general interface for managing asynchronous progress.
 * The async things are a collection items that needs progress. Each
 * "thing" is opaque with all the logic hidden inside the "poll_fn"
 * callback. The callback is passed with the entry pointer itself, thus
 * it can -
 *     * update the entry as a state machine.
 *     * cleanup the entry states as it progresses.
 *     * return a new list of new entries to be progressed.
 *     * return a done state for the entry to be removed.
 *
 * Each async things are independent items may be progressed in any order.
 * A single MPI_Test may progress all or just a few of the items. We only promise
 * to progress all items eventually after repeated MPI_Test calls.
 */

struct MPIR_Async_thing {
    int (*poll_fn) (struct MPIR_Async_thing * entry);
    void *state;
    MPIR_Stream *stream_ptr;
    /* doubly-linked list */
    struct MPIR_Async_thing *next, *prev;
    /* poll_fn may add new async thing entries */
    struct MPIR_Async_thing *new_entries;
};

/* two access functions to make MPIX_Async_thing opaque */
static inline void *MPIR_Async_thing_get_state(MPIX_Async_thing thing)
{
    return thing->state;
}

static inline int MPIR_Async_thing_spawn(struct MPIR_Async_thing *thing,
                                         MPIX_Async_poll_function poll_fn, void *state,
                                         MPIR_Stream * stream_ptr)
{
    struct MPIR_Async_thing *entry = MPL_malloc(sizeof(struct MPIR_Async_thing), MPL_MEM_OTHER);
    entry->poll_fn = poll_fn;
    entry->state = state;
    entry->stream_ptr = stream_ptr;
    entry->new_entries = NULL;

    DL_APPEND(thing->new_entries, entry);

    return MPI_SUCCESS;
}

int MPIR_Async_things_init(void);
int MPIR_Async_things_finalize(void);
int MPIR_Async_things_add(MPIX_Async_poll_function poll_fn, void *state, MPIR_Stream * stream_ptr);
int MPIR_Async_things_progress(int vci, int *made_progress);

#endif /* MPIR_ASYNC_THINGS_H_INCLUDED */
