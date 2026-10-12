/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

/* TODO: dynamic process support is not implemented yet */

int MPIDI_SOCK_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_dynamic_recv(int tag, void *buf, int size, int timeout)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_dynamic_sendrecv(MPIR_Lpid remote_lpid, MPIR_Comm * peer_comm, int tag,
                                const void *send_buf, int send_size, void *recv_buf, int recv_size,
                                int timeout)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_get_local_upids(MPIR_Comm * comm, int **local_upid_size, char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_SOCK_insert_upid(MPIR_Lpid lpid, const char *upid, int upid_len)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
