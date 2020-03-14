/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#undef utarray_oom
#define utarray_oom() do { goto fn_oom; } while (0)
#include "utarray.h"

#define NULL_CONTEXT_ID -1

int MPIDI_CH3I_comm_create(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_CREATE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_COMM_CREATE);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_COMM_CREATE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3I_comm_destroy(MPIR_Comm *comm, void *param)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
    
 fn_exit: ATTRIBUTE((unused))
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_COMM_DESTROY);
    return mpi_errno;
}

static int nem_coll_finalize(void *param ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NEM_COLL_FINALIZE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NEM_COLL_FINALIZE);

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NEM_COLL_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


int MPID_nem_coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_COLL_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_COLL_INIT);

    MPIR_Add_finalize(nem_coll_finalize, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);
    
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_COLL_INIT);
    return mpi_errno;
 fn_oom: /* out-of-memory handler for utarray operations */
    MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "utarray");
 fn_fail:
    goto fn_exit;
}

