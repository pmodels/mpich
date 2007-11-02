/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_make_progress()
{
    MPIDI_STATE_DECL(MPID_STATE_MM_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_MAKE_PROGRESS);
    
    packer_make_progress();
#ifdef WITH_METHOD_TCP
    tcp_make_progress();
#endif
#ifdef WITH_METHOD_SOCKET
    socket_make_progress();
#endif
#ifdef WITH_METHOD_SHM
    shm_make_progress();
#endif
#ifdef WITH_METHOD_IB
    ib_make_progress();
#endif
#ifdef WITH_METHOD_VIA
    via_make_progress();
#endif
#ifdef WITH_METHOD_VIA_RDMA
    via_rdma_make_progress();
#endif
#ifdef WITH_METHOD_NEW
    new_make_progress();
#endif
    unpacker_make_progress();

    MPIDI_FUNC_EXIT(MPID_STATE_MM_MAKE_PROGRESS);
    return MPI_SUCCESS;
}
