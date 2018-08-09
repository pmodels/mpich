/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_MISC_H_INCLUDED
#define SHM_MISC_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr, bool is_remote)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);

    ret = MPIDI_POSIX_comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_COMM_GET_LPID);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_node_id(MPIR_Comm * comm, int rank, int *id_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_GET_NODE_ID);

    ret = MPIDI_POSIX_get_node_id(comm, rank, id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_GET_NODE_ID);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_max_node_id(MPIR_Comm * comm, int *max_id_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_GET_MAX_NODE_ID);

    ret = MPIDI_POSIX_get_max_node_id(comm, max_id_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_GET_MAX_NODE_ID);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                                       char **local_upids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);

    ret = MPIDI_POSIX_get_local_upids(comm, local_upid_size, local_upids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_GET_LOCAL_UPIDS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_upids_to_lupids(int size, size_t * remote_upid_size,
                                                       char *remote_upids, int **remote_lupids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);

    ret = MPIDI_POSIX_upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_UPIDS_TO_LUPIDS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                   int size, const int lpids[])
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);

    ret = MPIDI_POSIX_create_intercomm_from_lpids(newcomm_ptr, size, lpids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_CREATE_INTERCOMM_FROM_LPIDS);
    return ret;
}

#endif /* SHM_MISC_H_INCLUDED */
