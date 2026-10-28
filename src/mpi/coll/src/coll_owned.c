/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

void MPIR_Coll_owned_init(MPIR_Coll_owned_t * o)
{
    utarray_init(&o->buffers, &ut_ptr_icd);
    utarray_init(&o->handles, &ut_int_icd);
}

void MPIR_Coll_owned_add_buffer(MPIR_Coll_owned_t * o, void *buf)
{
    utarray_push_back(&o->buffers, &buf, MPL_MEM_COLL);
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
    utarray_push_back(&o->handles, &handle, MPL_MEM_COLL);
}

void MPIR_Coll_owned_free(MPIR_Coll_owned_t * o)
{
    for (int *p = (int *)utarray_front(&o->handles); p; p = (int *) utarray_next(&o->handles, p)) {
        switch (HANDLE_GET_MPI_KIND(*p)) {
            case MPIR_COMM:
                {
                    MPIR_Comm *comm;
                    MPIR_Comm_get_ptr(*p, comm);
                    MPIR_Comm_release(comm);
                }
                break;
            case MPIR_DATATYPE:
                MPIR_Datatype_release_if_not_builtin(*p);
                break;
            case MPIR_OP:
                MPIR_Op_release_if_not_builtin(*p);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    utarray_done(&o->handles);
    for (void **p = (void **)utarray_front(&o->buffers); p;
         p = (void **) utarray_next(&o->buffers, p)) {
        MPL_free(*p);
    }
    utarray_done(&o->buffers);
}
