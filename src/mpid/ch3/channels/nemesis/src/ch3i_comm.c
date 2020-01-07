/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#undef utarray_oom
#define utarray_oom() do { goto fn_oom; } while (0)
#include "utarray.h"

#define NULL_CONTEXT_ID -1

int MPIDI_CH3I_comm_create(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3I_comm_destroy(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;

    
 fn_exit: ATTRIBUTE((unused))
    return mpi_errno;
}

static int nem_coll_finalize(void *param ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPID_nem_coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Add_finalize(nem_coll_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);
    
 fn_exit:
    return mpi_errno;
 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    goto fn_exit;
}

