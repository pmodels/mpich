/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"

void MPIR_Coll_owned_init(MPIR_Coll_owned_t * o)
{
    o->n_buffers = 0;
    o->max_buffers = 0;
    o->heap_buffers = NULL;
    o->n_handles = 0;
    o->max_handles = 0;
    o->heap_handles = NULL;
}

/* Grows a spilled array (or spills an inline one) to hold one more element of size sz */
static void *owned_grow(void *heap, int *max, int n, const void *inl, size_t sz)
{
    int new_max = *max ? 2 * *max : 4 * MPIR_COLL_OWNED_INLINE;
    void *p = MPL_realloc(heap, new_max * sz, MPL_MEM_COLL);
    MPIR_Assertp(p);
    if (!heap) {
        memcpy(p, inl, n * sz);
    }
    *max = new_max;
    return p;
}

void MPIR_Coll_owned_add_buffer(MPIR_Coll_owned_t * o, void *buf)
{
    if (!o->heap_buffers && o->n_buffers < MPIR_COLL_OWNED_INLINE) {
        o->inl_buffers[o->n_buffers++] = buf;
        return;
    }
    if (!o->heap_buffers || o->n_buffers == o->max_buffers) {
        o->heap_buffers = owned_grow(o->heap_buffers, &o->max_buffers, o->n_buffers,
                                     o->inl_buffers, sizeof(void *));
    }
    o->heap_buffers[o->n_buffers++] = buf;
}

/* Take a reference of our own on a communicator, datatype or op. A builtin
 * datatype or op has no reference count. A window is rejected, since freeing
 * one is collective. */
void MPIR_Coll_owned_add_handle(MPIR_Coll_owned_t * o, int handle)
{
    switch (HANDLE_GET_MPI_KIND(handle)) {
        case MPIR_COMM:
            {
                MPIR_Comm *comm;
                MPIR_Comm_get_ptr(handle, comm);
                MPIR_Comm_add_ref(comm);
            }
            break;
        case MPIR_DATATYPE:
            if (HANDLE_IS_BUILTIN(handle)) {
                return;
            }
            MPIR_Datatype_add_ref_if_not_builtin(handle);
            break;
        case MPIR_OP:
            if (HANDLE_IS_BUILTIN(handle)) {
                return;
            }
            MPIR_Op_add_ref_if_not_builtin(handle);
            break;
        default:
            MPIR_Assert(0);
    }
    if (!o->heap_handles && o->n_handles < MPIR_COLL_OWNED_INLINE) {
        o->inl_handles[o->n_handles++] = handle;
        return;
    }
    if (!o->heap_handles || o->n_handles == o->max_handles) {
        o->heap_handles = owned_grow(o->heap_handles, &o->max_handles, o->n_handles,
                                     o->inl_handles, sizeof(int));
    }
    o->heap_handles[o->n_handles++] = handle;
}

void MPIR_Coll_owned_free(MPIR_Coll_owned_t * o)
{
    int *handles = o->heap_handles ? o->heap_handles : o->inl_handles;
    for (int i = 0; i < o->n_handles; i++) {
        switch (HANDLE_GET_MPI_KIND(handles[i])) {
            case MPIR_COMM:
                {
                    MPIR_Comm *comm;
                    MPIR_Comm_get_ptr(handles[i], comm);
                    MPIR_Comm_release(comm);
                }
                break;
            case MPIR_DATATYPE:
                MPIR_Datatype_release_if_not_builtin(handles[i]);
                break;
            case MPIR_OP:
                MPIR_Op_release_if_not_builtin(handles[i]);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    void **buffers = o->heap_buffers ? o->heap_buffers : o->inl_buffers;
    for (int i = 0; i < o->n_buffers; i++) {
        MPL_free(buffers[i]);
    }
    if (o->heap_handles) {
        MPL_free(o->heap_handles);
    }
    if (o->heap_buffers) {
        MPL_free(o->heap_buffers);
    }
    MPIR_Coll_owned_init(o);
}
