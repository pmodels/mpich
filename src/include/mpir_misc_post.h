/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_MISC_POST_H_INCLUDED
#define MPIR_MISC_POST_H_INCLUDED

/* Pull the error status out of the tag space and return status->MPI_ERROR */
static inline int MPIR_Process_status(MPI_Status * status)
{
    if (MPI_PROC_NULL != status->MPI_SOURCE && MPIR_TAG_CHECK_ERROR_BIT(status->MPI_TAG)) {
        int err = MPI_SUCCESS;
        if (MPIR_TAG_CHECK_PROC_FAILURE_BIT(status->MPI_TAG)) {
            MPIR_ERR_SET(err, MPIX_ERR_PROC_FAILED, "**fail");
        } else {
            MPIR_ERR_SET(err, MPI_ERR_OTHER, "**fail");
        }
        MPIR_ERR_ADD(status->MPI_ERROR, err);
        MPIR_TAG_CLEAR_ERROR_BITS(status->MPI_TAG);
    }
    return status->MPI_ERROR;
}

/* Take or drop a reference on an object by its handle, for a holder that keeps handles of
 * different kinds (communicator, datatype, op). Predefined handles are skipped. */
static inline void MPIR_Handle_add_ref(int handle)
{
    switch (HANDLE_GET_MPI_KIND(handle)) {
        case MPIR_COMM:
            {
                MPIR_Comm *comm_ptr;
                MPIR_Comm_get_ptr(handle, comm_ptr);
                MPIR_Comm_add_ref(comm_ptr);
                break;
            }
        case MPIR_DATATYPE:
            MPIR_Datatype_add_ref_if_not_builtin(handle);
            break;
        case MPIR_OP:
            MPIR_Op_add_ref_if_not_builtin(handle);
            break;
        default:
            MPIR_Assert(0);
    }
}

static inline void MPIR_Handle_release(int handle)
{
    switch (HANDLE_GET_MPI_KIND(handle)) {
        case MPIR_COMM:
            {
                MPIR_Comm *comm_ptr;
                MPIR_Comm_get_ptr(handle, comm_ptr);
                MPIR_Comm_release(comm_ptr);
                break;
            }
        case MPIR_DATATYPE:
            MPIR_Datatype_release_if_not_builtin(handle);
            break;
        case MPIR_OP:
            MPIR_Op_release_if_not_builtin(handle);
            break;
        default:
            MPIR_Assert(0);
    }
}

#endif /* MPIR_MISC_POST_H_INCLUDED */
