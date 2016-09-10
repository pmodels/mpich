/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <assert.h>
#include "mpiimpl.h"

int MPIR_Nsched_progress_hook_id;

int MPIR_Nsched_init(void)
{
    MPID_Progress_register_hook(MPIR_Nsched_progress_hook, &MPIR_Nsched_progress_hook_id);

    return MPI_SUCCESS;
}

int MPIR_Nsched_finalize(void)
{
    MPID_Progress_deregister_hook(MPIR_Nsched_progress_hook_id);

    return MPI_SUCCESS;
}
