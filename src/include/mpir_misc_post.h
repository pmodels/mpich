/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_MISC_POST_H_INCLUDED
#define MPIR_MISC_POST_H_INCLUDED

/* Pull the error status out of the tag space and put it into an errflag. */
#undef FUNCNAME
#define FUNCNAME MPIR_process_status
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIR_Process_status(MPI_Status * status, MPIR_Errflag_t * errflag)
{
    if (MPI_PROC_NULL != status->MPI_SOURCE &&
        (MPIX_ERR_REVOKED == MPIR_ERR_GET_CLASS(status->MPI_ERROR) ||
         MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(status->MPI_ERROR) ||
         MPIR_TAG_CHECK_ERROR_BIT(status->MPI_TAG)) && !*errflag) {
        /* If the receive was completed within the MPID_Recv, handle the
         * errflag here. */
        if (MPIR_TAG_CHECK_PROC_FAILURE_BIT(status->MPI_TAG) ||
            MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(status->MPI_ERROR)) {
            *errflag = MPIR_ERR_PROC_FAILED;
            MPIR_TAG_CLEAR_ERROR_BITS(status->MPI_TAG);
        } else {
            *errflag = MPIR_ERR_OTHER;
            MPIR_TAG_CLEAR_ERROR_BITS(status->MPI_TAG);
        }
    }
}

#endif /* MPIR_MISC_POST_H_INCLUDED */
