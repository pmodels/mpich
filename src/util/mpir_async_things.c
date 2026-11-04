/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpir_async_things.h"

/* Each list has one thread polling it at a time, and that thread holds no lock while it polls, so a
 * poll_fn may take any lock or start a schedule. A thread that finds the list being polled skips it
 * instead of waiting, so polling adds no lock ordering, and a poll_fn that calls progress skips too.
 * Adding takes no lock either, so any context can add under any lock: the new thing goes on a
 * stack, and the polling thread moves it to the list.
 *
 * Only VCI granularity with threads needs real atomics. Otherwise there is either one thread or the
 * global lock is held, and a poll_fn that yields the global lock still leaves polling set. */
struct async_list {
    /* only the polling thread touches it */
    struct MPIR_Async_thing *list;
    MPL_atomic_int_t polling;
    int hook_id;
    /* a stack linked by next, on its own cache line since other threads write it */
    MPL_atomic_ptr_t incoming MPL_ATTR_ALIGNED(MPL_CACHELINE_SIZE);
} MPL_ATTR_ALIGNED(MPL_CACHELINE_SIZE);

static struct async_list async_lists[MPIR_MAX_VCIS + 1];

#if defined(MPICH_IS_THREADED) && MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
#define USE_ATOMICS() (MPIR_ThreadInfo.isThreaded)
#else
#define USE_ATOMICS() 0
#endif

static bool poll_try_acquire(struct async_list *l);
static void poll_release(struct async_list *l);
static void push_incoming(struct async_list *l, struct MPIR_Async_thing *entry);
static void splice_incoming(struct async_list *l);

int MPIR_Async_things_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        struct async_list *l = &async_lists[vci];
        l->list = NULL;
        MPL_atomic_relaxed_store_int(&l->polling, 0);
        MPL_atomic_relaxed_store_ptr(&l->incoming, NULL);

        /* vci == MPIR_MAX_VCIS is equivalent to -1 */
        mpi_errno = MPIR_Progress_hook_register(vci, MPIR_Async_things_progress, &l->hook_id);
    }
    return mpi_errno;
}

int MPIR_Async_things_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        struct async_list *l = &async_lists[vci];
        while (l->list != NULL || MPL_atomic_relaxed_load_ptr(&l->incoming) != NULL) {
            MPID_Progress_test(NULL);
        }
    }

    for (int vci = 0; vci < MPIR_MAX_VCIS + 1; vci++) {
        mpi_errno = MPIR_Progress_hook_deregister(async_lists[vci].hook_id);
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

    struct async_list *l = &async_lists[vci];
    push_incoming(l, entry);
    /* pairs with the barrier in MPIR_Async_things_progress */
    if (USE_ATOMICS()) {
        MPL_atomic_read_write_barrier();
    }
    MPIR_Progress_hook_activate(l->hook_id);

    return MPI_SUCCESS;
}

int MPIR_Async_things_add_in_vci(int (*poll_fn) (struct MPIR_Async_thing * entry), void *state,
                                 MPIR_Stream * stream_ptr, int vci)
{
    return MPIR_Async_things_add(poll_fn, state, stream_ptr);
}

/* For ch3, which has no streams: whether it may block in its progress */
int MPIR_Async_things_pending(void)
{
    return MPIR_Progress_hook_is_active(async_lists[MPIR_MAX_VCIS].hook_id);
}

int MPIR_Async_things_progress(int vci, int *made_progress)
{
    if (vci == -1) {
        vci = MPIR_MAX_VCIS;
    }
    struct async_list *l = &async_lists[vci];

    if (!poll_try_acquire(l)) {
        return MPI_SUCCESS;
    }

    splice_incoming(l);

    struct MPIR_Async_thing *entry, *tmp;
    DL_FOREACH_SAFE(l->list, entry, tmp) {
        int ret = entry->poll_fn(entry);
        if (entry->new_entries) {
            *made_progress = 1;
            DL_CONCAT(l->list, entry->new_entries);
            entry->new_entries = NULL;
        }
        if (ret == MPIX_ASYNC_DONE) {
            *made_progress = 1;
            DL_DELETE(l->list, entry);
            MPL_free(entry);
        }
    }

    if (l->list == NULL) {
        MPIR_Progress_hook_deactivate(l->hook_id);
        /* A store followed by a load here and in MPIR_Async_things_add: either we see the new
         * thing, or its activation comes after our deactivation. */
        if (USE_ATOMICS()) {
            MPL_atomic_read_write_barrier();
        }
        if (MPL_atomic_relaxed_load_ptr(&l->incoming) != NULL) {
            MPIR_Progress_hook_activate(l->hook_id);
        }
    }

    poll_release(l);

    return MPI_SUCCESS;
}

/* test-and-test-and-set (TTAS) try-lock */
static bool poll_try_acquire(struct async_list *l)
{
    /* a thread that will skip does not write the cache line */
    if (MPL_atomic_relaxed_load_int(&l->polling)) {
        return false;
    }
    if (USE_ATOMICS()) {
        return MPL_atomic_cas_int(&l->polling, 0, 1) == 0;
    }
    MPL_atomic_relaxed_store_int(&l->polling, 1);
    return true;
}

static void poll_release(struct async_list *l)
{
    if (USE_ATOMICS()) {
        MPL_atomic_release_store_int(&l->polling, 0);
    } else {
        MPL_atomic_relaxed_store_int(&l->polling, 0);
    }
}

/* lock-free MPSC LIFO */
static void push_incoming(struct async_list *l, struct MPIR_Async_thing *entry)
{
    if (USE_ATOMICS()) {
        void *head;
        do {
            head = MPL_atomic_relaxed_load_ptr(&l->incoming);
            entry->next = head;
        } while (MPL_atomic_cas_ptr(&l->incoming, head, entry) != head);
    } else {
        entry->next = MPL_atomic_relaxed_load_ptr(&l->incoming);
        MPL_atomic_relaxed_store_ptr(&l->incoming, entry);
    }
}

/* Moves the incoming things to the end of the list, in the order they were added */
static void splice_incoming(struct async_list *l)
{
    if (MPL_atomic_relaxed_load_ptr(&l->incoming) == NULL) {
        return;
    }

    struct MPIR_Async_thing *head;
    if (USE_ATOMICS()) {
        head = MPL_atomic_swap_ptr(&l->incoming, NULL);
    } else {
        head = MPL_atomic_relaxed_load_ptr(&l->incoming);
        MPL_atomic_relaxed_store_ptr(&l->incoming, NULL);
    }

    struct MPIR_Async_thing *batch = NULL;
    while (head) {
        struct MPIR_Async_thing *next = head->next;
        DL_PREPEND(batch, head);
        head = next;
    }
    DL_CONCAT(l->list, batch);
}
