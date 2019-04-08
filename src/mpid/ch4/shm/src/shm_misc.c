/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size, char **local_upids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_GET_LOCAL_UPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_GET_LOCAL_UPIDS);

    ret = MPIDI_POSIX_get_local_upids(comm, local_upid_size, local_upids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_GET_LOCAL_UPIDS);
    return ret;
}

int MPIDI_SHMI_upids_to_lupids(int size, size_t * remote_upid_size, char *remote_upids,
                               int **remote_lupids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_UPIDS_TO_LUPIDS);

    ret = MPIDI_POSIX_upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_UPIDS_TO_LUPIDS);
    return ret;
}

int MPIDI_SHMI_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr, int size, const int lpids[])
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_CREATE_INTERCOMM_FROM_LPIDS);

    ret = MPIDI_POSIX_create_intercomm_from_lpids(newcomm_ptr, size, lpids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_CREATE_INTERCOMM_FROM_LPIDS);
    return ret;
}
