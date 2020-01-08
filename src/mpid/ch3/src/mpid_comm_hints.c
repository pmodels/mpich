/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int set_eager_threshold(MPIR_Comm *comm_ptr, MPIR_Info *info, void *state);

static int set_eager_threshold(MPIR_Comm *comm_ptr, MPIR_Info *info, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    char *endptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIDI_CH3_SET_EAGER_THRESHOLD);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIDI_CH3_SET_EAGER_THRESHOLD);

    comm_ptr->dev.eager_max_msg_sz = strtol(info->value, &endptr, 0);

    MPIR_ERR_CHKANDJUMP1(*endptr, mpi_errno, MPI_ERR_ARG,
                         "**infohintparse", "**infohintparse %s",
                         info->key);

 fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIDI_CH3_SET_EAGER_THRESHOLD);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* AUTOGEN-MPID_CH3_INIT */
int init_ch3_comm_hints(void);
int init_ch3_comm_hints(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Comm_register_hint("eager_rendezvous_threshold",
                                        set_eager_threshold,
                                        NULL);
    MPIR_ERR_CHECK(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
