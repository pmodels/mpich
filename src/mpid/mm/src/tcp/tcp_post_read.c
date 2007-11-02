/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_TCP

int tcp_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_POST_READ);
    tcp_car_enqueue(vc_ptr, car_ptr);
    MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_READ);
    return MPI_SUCCESS;
}

int tcp_post_read_pkt(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_TCP_POST_READ_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_TCP_POST_READ_PKT);

#ifdef MPICH_DEV_BUILD
    if (!vc_ptr->data.tcp.connected)
    {
	err_printf("Error: tcp_post_read_pkt cannot change to reading_header state until the vc is connected.\n");
    }
#endif
    vc_ptr->data.tcp.bytes_of_header_read = 0;
    vc_ptr->data.tcp.read = tcp_read_header;

    MPIDI_FUNC_EXIT(MPID_STATE_TCP_POST_READ_PKT);
    return MPI_SUCCESS;
}

#endif
