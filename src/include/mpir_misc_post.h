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

#endif /* MPIR_MISC_POST_H_INCLUDED */
